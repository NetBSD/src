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
#define yyparse rl78_parse
#define yylex   rl78_lex
#define yyerror rl78_error
#define yylval  rl78_lval
#define yychar  rl78_char
#define yydebug rl78_debug
#define yynerrs rl78_nerrs


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     A = 258,
     X = 259,
     B = 260,
     C = 261,
     D = 262,
     E = 263,
     H = 264,
     L = 265,
     AX = 266,
     BC = 267,
     DE = 268,
     HL = 269,
     SPL = 270,
     SPH = 271,
     PSW = 272,
     CS = 273,
     ES = 274,
     PMC = 275,
     MEM = 276,
     FLAG = 277,
     SP = 278,
     CY = 279,
     RB0 = 280,
     RB1 = 281,
     RB2 = 282,
     RB3 = 283,
     EXPR = 284,
     UNKNOWN_OPCODE = 285,
     IS_OPCODE = 286,
     DOT_S = 287,
     DOT_B = 288,
     DOT_W = 289,
     DOT_L = 290,
     DOT_A = 291,
     DOT_UB = 292,
     DOT_UW = 293,
     ADD = 294,
     ADDC = 295,
     ADDW = 296,
     AND_ = 297,
     AND1 = 298,
     BF = 299,
     BH = 300,
     BNC = 301,
     BNH = 302,
     BNZ = 303,
     BR = 304,
     BRK = 305,
     BRK1 = 306,
     BT = 307,
     BTCLR = 308,
     BZ = 309,
     CALL = 310,
     CALLT = 311,
     CLR1 = 312,
     CLRB = 313,
     CLRW = 314,
     CMP = 315,
     CMP0 = 316,
     CMPS = 317,
     CMPW = 318,
     DEC = 319,
     DECW = 320,
     DI = 321,
     DIVHU = 322,
     DIVWU = 323,
     EI = 324,
     HALT = 325,
     INC = 326,
     INCW = 327,
     MACH = 328,
     MACHU = 329,
     MOV = 330,
     MOV1 = 331,
     MOVS = 332,
     MOVW = 333,
     MULH = 334,
     MULHU = 335,
     MULU = 336,
     NOP = 337,
     NOT1 = 338,
     ONEB = 339,
     ONEW = 340,
     OR = 341,
     OR1 = 342,
     POP = 343,
     PUSH = 344,
     RET = 345,
     RETI = 346,
     RETB = 347,
     ROL = 348,
     ROLC = 349,
     ROLWC = 350,
     ROR = 351,
     RORC = 352,
     SAR = 353,
     SARW = 354,
     SEL = 355,
     SET1 = 356,
     SHL = 357,
     SHLW = 358,
     SHR = 359,
     SHRW = 360,
     SKC = 361,
     SKH = 362,
     SKNC = 363,
     SKNH = 364,
     SKNZ = 365,
     SKZ = 366,
     STOP = 367,
     SUB = 368,
     SUBC = 369,
     SUBW = 370,
     XCH = 371,
     XCHW = 372,
     XOR = 373,
     XOR1 = 374
   };
#endif
/* Tokens.  */
#define A 258
#define X 259
#define B 260
#define C 261
#define D 262
#define E 263
#define H 264
#define L 265
#define AX 266
#define BC 267
#define DE 268
#define HL 269
#define SPL 270
#define SPH 271
#define PSW 272
#define CS 273
#define ES 274
#define PMC 275
#define MEM 276
#define FLAG 277
#define SP 278
#define CY 279
#define RB0 280
#define RB1 281
#define RB2 282
#define RB3 283
#define EXPR 284
#define UNKNOWN_OPCODE 285
#define IS_OPCODE 286
#define DOT_S 287
#define DOT_B 288
#define DOT_W 289
#define DOT_L 290
#define DOT_A 291
#define DOT_UB 292
#define DOT_UW 293
#define ADD 294
#define ADDC 295
#define ADDW 296
#define AND_ 297
#define AND1 298
#define BF 299
#define BH 300
#define BNC 301
#define BNH 302
#define BNZ 303
#define BR 304
#define BRK 305
#define BRK1 306
#define BT 307
#define BTCLR 308
#define BZ 309
#define CALL 310
#define CALLT 311
#define CLR1 312
#define CLRB 313
#define CLRW 314
#define CMP 315
#define CMP0 316
#define CMPS 317
#define CMPW 318
#define DEC 319
#define DECW 320
#define DI 321
#define DIVHU 322
#define DIVWU 323
#define EI 324
#define HALT 325
#define INC 326
#define INCW 327
#define MACH 328
#define MACHU 329
#define MOV 330
#define MOV1 331
#define MOVS 332
#define MOVW 333
#define MULH 334
#define MULHU 335
#define MULU 336
#define NOP 337
#define NOT1 338
#define ONEB 339
#define ONEW 340
#define OR 341
#define OR1 342
#define POP 343
#define PUSH 344
#define RET 345
#define RETI 346
#define RETB 347
#define ROL 348
#define ROLC 349
#define ROLWC 350
#define ROR 351
#define RORC 352
#define SAR 353
#define SARW 354
#define SEL 355
#define SET1 356
#define SHL 357
#define SHLW 358
#define SHR 359
#define SHRW 360
#define SKC 361
#define SKH 362
#define SKNC 363
#define SKNH 364
#define SKNZ 365
#define SKZ 366
#define STOP 367
#define SUB 368
#define SUBC 369
#define SUBW 370
#define XCH 371
#define XCHW 372
#define XOR 373
#define XOR1 374




/* Copy the first part of user declarations.  */
#line 21 "rl78-parse.y"


#include "as.h"
#include "safe-ctype.h"
#include "rl78-defs.h"

static int rl78_lex (void);

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

#define B1(b1)             rl78_base1 (b1)
#define B2(b1, b2)         rl78_base2 (b1, b2)
#define B3(b1, b2, b3)     rl78_base3 (b1, b2, b3)
#define B4(b1, b2, b3, b4) rl78_base4 (b1, b2, b3, b4)

/* POS is bits from the MSB of the first byte to the LSB of the last byte.  */
#define F(val,pos,sz)      rl78_field (val, pos, sz)
#define FE(exp,pos,sz)	   rl78_field (exp_val (exp), pos, sz);

#define O1(v)              rl78_op (v, 1, RL78REL_DATA)
#define O2(v)              rl78_op (v, 2, RL78REL_DATA)
#define O3(v)              rl78_op (v, 3, RL78REL_DATA)
#define O4(v)              rl78_op (v, 4, RL78REL_DATA)

#define PC1(v)             rl78_op (v, 1, RL78REL_PCREL)
#define PC2(v)             rl78_op (v, 2, RL78REL_PCREL)
#define PC3(v)             rl78_op (v, 3, RL78REL_PCREL)

#define IMM(v,pos)	   F (immediate (v, RL78REL_SIGNED, pos), pos, 2); \
			   if (v.X_op != O_constant && v.X_op != O_big) rl78_linkrelax_imm (pos)
#define NIMM(v,pos)	   F (immediate (v, RL78REL_NEGATIVE, pos), pos, 2)
#define NBIMM(v,pos)	   F (immediate (v, RL78REL_NEGATIVE_BORROW, pos), pos, 2)
#define DSP(v,pos,msz)	   if (!v.X_md) rl78_relax (RL78_RELAX_DISP, pos); \
			   else rl78_linkrelax_dsp (pos); \
			   F (displacement (v, msz), pos, 2)

#define id24(a,b2,b3)	   B3 (0xfb+a, b2, b3)

static int         expr_is_sfr (expressionS);
static int         expr_is_saddr (expressionS);
static int         expr_is_word_aligned (expressionS);
static int         exp_val (expressionS exp);

static int    need_flag = 0;
static int    rl78_in_brackets = 0;
static int    rl78_last_token = 0;
static char * rl78_init_start;
static char * rl78_last_exp_start = 0;
static int    rl78_bit_insn = 0;

#define YYDEBUG 1
#define YYERROR_VERBOSE 1

#define NOT_SADDR  rl78_error ("Expression not 0xFFE20 to 0xFFF1F")
#define SA(e) if (!expr_is_saddr (e)) NOT_SADDR;

#define NOT_SFR  rl78_error ("Expression not 0xFFF00 to 0xFFFFF")
#define SFR(e) if (!expr_is_sfr (e)) NOT_SFR;

#define NOT_SFR_OR_SADDR  rl78_error ("Expression not 0xFFE20 to 0xFFFFF")

#define NOT_ES if (rl78_has_prefix()) rl78_error ("ES: prefix not allowed here");

#define WA(x) if (!expr_is_word_aligned (x)) rl78_error ("Expression not word-aligned");

static void check_expr_is_bit_index (expressionS);
#define Bit(e) check_expr_is_bit_index (e);

/* Returns TRUE (non-zero) if the expression is a constant in the
   given range.  */
static int check_expr_is_const (expressionS, int vmin, int vmax);

/* Convert a "regb" value to a "reg_xbc" value.  Error if other
   registers are passed.  Needed to avoid reduce-reduce conflicts.  */
static int
reg_xbc (int reg)
{
  switch (reg)
    {
      case 0: /* X */
        return 0x10;
      case 3: /* B */
        return 0x20;
      case 2: /* C */
        return 0x30;
      default:
        rl78_error ("Only X, B, or C allowed here");
	return 0;
    }
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

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif

#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
#line 139 "rl78-parse.y"
{
  int regno;
  expressionS exp;
}
/* Line 193 of yacc.c.  */
#line 463 "rl78-parse.c"
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 216 of yacc.c.  */
#line 476 "rl78-parse.c"

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
#define YYFINAL  174
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   835

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  129
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  50
/* YYNRULES -- Number of rules.  */
#define YYNRULES  318
/* YYNRULES -- Number of states.  */
#define YYNSTATES  738

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   374

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   122,     2,   121,   127,     2,     2,     2,
       2,     2,     2,   125,   120,     2,   126,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,   128,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   123,     2,   124,     2,     2,     2,     2,     2,     2,
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
     105,   106,   107,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   118,   119
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     5,    11,    12,    19,    24,    29,    34,
      35,    41,    48,    56,    66,    76,    86,    94,   100,   105,
     106,   112,   119,   129,   137,   143,   144,   152,   153,   161,
     162,   170,   171,   182,   186,   190,   194,   198,   202,   206,
     214,   222,   230,   241,   244,   248,   253,   257,   262,   264,
     266,   269,   274,   278,   283,   288,   291,   296,   301,   306,
     313,   321,   324,   327,   330,   333,   334,   338,   343,   346,
     349,   352,   355,   358,   361,   362,   366,   371,   381,   384,
     385,   389,   393,   399,   406,   415,   418,   419,   423,   428,
     436,   438,   440,   442,   444,   447,   449,   451,   453,   455,
     457,   463,   469,   475,   476,   484,   491,   500,   505,   510,
     511,   518,   525,   531,   539,   546,   547,   554,   555,   556,
     564,   569,   574,   575,   576,   584,   592,   600,   611,   621,
     631,   639,   647,   658,   668,   678,   688,   698,   708,   718,
     728,   737,   746,   756,   765,   774,   784,   793,   802,   810,
     819,   827,   828,   840,   841,   851,   852,   863,   864,   873,
     874,   885,   886,   895,   902,   909,   916,   926,   933,   940,
     947,   957,   967,   973,   979,   980,   988,   989,   996,   997,
    1004,  1009,  1014,  1021,  1028,  1036,  1044,  1054,  1064,  1072,
    1080,  1090,  1100,  1109,  1118,  1127,  1136,  1145,  1153,  1162,
    1170,  1171,  1182,  1183,  1192,  1193,  1204,  1205,  1214,  1215,
    1221,  1228,  1234,  1239,  1244,  1249,  1251,  1254,  1257,  1260,
    1263,  1266,  1268,  1270,  1272,  1277,  1282,  1287,  1292,  1297,
    1302,  1307,  1312,  1315,  1318,  1321,  1324,  1329,  1334,  1339,
    1344,  1349,  1354,  1359,  1361,  1363,  1365,  1367,  1369,  1371,
    1373,  1378,  1385,  1393,  1403,  1411,  1421,  1431,  1441,  1446,
    1451,  1452,  1455,  1457,  1459,  1461,  1463,  1465,  1467,  1469,
    1471,  1473,  1475,  1477,  1479,  1481,  1483,  1485,  1487,  1489,
    1491,  1493,  1495,  1497,  1499,  1501,  1503,  1505,  1507,  1509,
    1511,  1513,  1515,  1517,  1519,  1521,  1523,  1525,  1527,  1529,
    1531,  1533,  1535,  1537,  1539,  1541,  1543,  1545,  1547,  1549,
    1551,  1553,  1555,  1557,  1559,  1561,  1563,  1565,  1567
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
     130,     0,    -1,    30,    -1,   169,     3,   120,   121,    29,
      -1,    -1,   169,    29,   131,   120,   121,    29,    -1,   169,
       3,   120,     3,    -1,   169,     3,   120,   165,    -1,   169,
     165,   120,     3,    -1,    -1,   169,     3,   120,    29,   132,
      -1,   169,     3,   120,   163,   122,    29,    -1,   169,     3,
     120,   163,   123,    14,   124,    -1,   169,     3,   120,   163,
     123,    14,   125,    29,   124,    -1,   169,     3,   120,   163,
     123,    14,   125,     5,   124,    -1,   169,     3,   120,   163,
     123,    14,   125,     6,   124,    -1,   169,   163,   122,    29,
     120,   121,    29,    -1,   170,    11,   120,   121,    29,    -1,
     170,    11,   120,   166,    -1,    -1,   170,    11,   120,    29,
     133,    -1,   170,    11,   120,   163,   122,    29,    -1,   170,
      11,   120,   163,   123,    14,   125,    29,   124,    -1,   170,
      11,   120,   163,   123,    14,   124,    -1,   170,    23,   120,
     121,    29,    -1,    -1,   171,    24,   120,   168,   126,    29,
     134,    -1,    -1,   171,    24,   120,    29,   126,    29,   135,
      -1,    -1,   171,    24,   120,     3,   126,    29,   136,    -1,
      -1,   171,    24,   120,   163,   123,    14,   124,   126,    29,
     137,    -1,    12,   127,    29,    -1,    46,   127,    29,    -1,
      54,   127,    29,    -1,    48,   127,    29,    -1,    45,   127,
      29,    -1,    47,   127,    29,    -1,   172,   168,   126,    29,
     120,   127,    29,    -1,   172,    29,   126,    29,   120,   127,
      29,    -1,   172,     3,   126,    29,   120,   127,    29,    -1,
     172,   163,   123,    14,   124,   126,    29,   120,   127,    29,
      -1,    49,    11,    -1,    49,   127,    29,    -1,    49,   127,
     122,    29,    -1,    49,   122,    29,    -1,    49,   122,   122,
      29,    -1,    50,    -1,    51,    -1,    55,   166,    -1,    55,
     127,   122,    29,    -1,    55,   122,    29,    -1,    55,   122,
     122,    29,    -1,    56,   123,    29,   124,    -1,   173,    24,
      -1,   173,   168,   126,    29,    -1,   173,    29,   126,    29,
      -1,   173,     3,   126,    29,    -1,   173,   163,   122,    29,
     126,    29,    -1,   173,   163,   123,    14,   124,   126,    29,
      -1,   174,     3,    -1,   174,     4,    -1,   174,     5,    -1,
     174,     6,    -1,    -1,   174,    29,   138,    -1,   174,   163,
     122,    29,    -1,   175,    11,    -1,   175,    12,    -1,    61,
       3,    -1,    61,     4,    -1,    61,     5,    -1,    61,     6,
      -1,    -1,    61,    29,   139,    -1,    61,   163,   122,    29,
      -1,    62,     4,   120,   163,   123,    14,   125,    29,   124,
      -1,   176,   164,    -1,    -1,   176,    29,   140,    -1,   176,
     122,    29,    -1,   176,    19,   128,   122,    29,    -1,   176,
     123,    14,   125,    29,   124,    -1,   176,    19,   128,   123,
      14,   125,    29,   124,    -1,   177,   166,    -1,    -1,   177,
      29,   141,    -1,   177,   163,   122,    29,    -1,   177,   163,
     123,    14,   125,    29,   124,    -1,    66,    -1,    69,    -1,
      80,    -1,    79,    -1,    81,     4,    -1,    67,    -1,    68,
      -1,    74,    -1,    73,    -1,    70,    -1,    75,     3,   120,
     121,    29,    -1,    75,   165,   120,   121,    29,    -1,    75,
     168,   120,   121,    29,    -1,    -1,    75,   163,    29,   120,
     121,    29,   142,    -1,    75,   122,    29,   120,   121,    29,
      -1,    75,    19,   128,   122,    29,   120,   121,    29,    -1,
      75,   165,   120,     3,    -1,    75,     3,   120,   165,    -1,
      -1,    75,   163,    29,   120,     3,   143,    -1,    75,     3,
     120,   163,   122,    29,    -1,    75,   122,    29,   120,     3,
      -1,    75,    19,   128,   122,    29,   120,     3,    -1,    75,
     165,   120,   163,   122,    29,    -1,    -1,    75,     3,   120,
     163,    29,   144,    -1,    -1,    -1,    75,   165,   120,   163,
      29,   145,   146,    -1,    75,     3,   120,   168,    -1,    75,
     168,   120,   164,    -1,    -1,    -1,    75,   168,   120,   163,
      29,   147,   148,    -1,    75,     3,   120,   163,   123,    13,
     124,    -1,    75,   163,   123,    13,   124,   120,     3,    -1,
      75,   163,   123,    13,   125,    29,   124,   120,   121,    29,
      -1,    75,     3,   120,   163,   123,    13,   125,    29,   124,
      -1,    75,   163,   123,    13,   125,    29,   124,   120,     3,
      -1,    75,     3,   120,   163,   123,    14,   124,    -1,    75,
     163,   123,    14,   124,   120,     3,    -1,    75,   163,   123,
      14,   125,    29,   124,   120,   121,    29,    -1,    75,     3,
     120,   163,   123,    14,   125,    29,   124,    -1,    75,   163,
     123,    14,   125,    29,   124,   120,     3,    -1,    75,     3,
     120,   163,   123,    14,   125,     5,   124,    -1,    75,   163,
     123,    14,   125,     5,   124,   120,     3,    -1,    75,     3,
     120,   163,   123,    14,   125,     6,   124,    -1,    75,   163,
     123,    14,   125,     6,   124,   120,     3,    -1,    75,   163,
      29,   123,     5,   124,   120,   121,    29,    -1,    75,     3,
     120,   163,    29,   123,     5,   124,    -1,    75,   163,    29,
     123,     5,   124,   120,     3,    -1,    75,   163,    29,   123,
       6,   124,   120,   121,    29,    -1,    75,     3,   120,   163,
      29,   123,     6,   124,    -1,    75,   163,    29,   123,     6,
     124,   120,     3,    -1,    75,   163,    29,   123,    12,   124,
     120,   121,    29,    -1,    75,   163,   123,    12,   124,   120,
     121,    29,    -1,    75,     3,   120,   163,    29,   123,    12,
     124,    -1,    75,     3,   120,   163,   123,    12,   124,    -1,
      75,   163,    29,   123,    12,   124,   120,     3,    -1,    75,
     163,   123,    12,   124,   120,     3,    -1,    -1,    75,   163,
     123,    23,   125,    29,   124,   120,   121,    29,   149,    -1,
      -1,    75,   163,   123,    23,   124,   120,   121,    29,   150,
      -1,    -1,    75,     3,   120,   163,   123,    23,   125,    29,
     124,   151,    -1,    -1,    75,     3,   120,   163,   123,    23,
     124,   152,    -1,    -1,    75,   163,   123,    23,   125,    29,
     124,   120,     3,   153,    -1,    -1,    75,   163,   123,    23,
     124,   120,     3,   154,    -1,   178,    24,   120,    29,   126,
      29,    -1,   178,    24,   120,     3,   126,    29,    -1,   178,
      24,   120,   168,   126,    29,    -1,   178,    24,   120,   163,
     123,    14,   124,   126,    29,    -1,   178,    29,   126,    29,
     120,    24,    -1,   178,     3,   126,    29,   120,    24,    -1,
     178,   168,   126,    29,   120,    24,    -1,   178,   163,   123,
      14,   124,   126,    29,   120,    24,    -1,    77,   163,   123,
      14,   125,    29,   124,   120,     4,    -1,    78,    11,   120,
     121,    29,    -1,    78,   167,   120,   121,    29,    -1,    -1,
      78,   163,    29,   120,   121,    29,   155,    -1,    -1,    78,
      11,   120,   163,    29,   156,    -1,    -1,    78,   163,    29,
     120,    11,   157,    -1,    78,    11,   120,   167,    -1,    78,
     167,   120,    11,    -1,    78,    11,   120,   163,   122,    29,
      -1,    78,   163,   122,    29,   120,    11,    -1,    78,    11,
     120,   163,   123,    13,   124,    -1,    78,   163,   123,    13,
     124,   120,    11,    -1,    78,    11,   120,   163,   123,    13,
     125,    29,   124,    -1,    78,   163,   123,    13,   125,    29,
     124,   120,    11,    -1,    78,    11,   120,   163,   123,    14,
     124,    -1,    78,   163,   123,    14,   124,   120,    11,    -1,
      78,    11,   120,   163,   123,    14,   125,    29,   124,    -1,
      78,   163,   123,    14,   125,    29,   124,   120,    11,    -1,
      78,    11,   120,   163,    29,   123,     5,   124,    -1,    78,
     163,    29,   123,     5,   124,   120,    11,    -1,    78,    11,
     120,   163,    29,   123,     6,   124,    -1,    78,   163,    29,
     123,     6,   124,   120,    11,    -1,    78,    11,   120,   163,
      29,   123,    12,   124,    -1,    78,    11,   120,   163,   123,
      12,   124,    -1,    78,   163,    29,   123,    12,   124,   120,
      11,    -1,    78,   163,   123,    12,   124,   120,    11,    -1,
      -1,    78,    11,   120,   163,   123,    23,   125,    29,   124,
     158,    -1,    -1,    78,    11,   120,   163,   123,    23,   124,
     159,    -1,    -1,    78,   163,   123,    23,   125,    29,   124,
     120,    11,   160,    -1,    -1,    78,   163,   123,    23,   124,
     120,    11,   161,    -1,    -1,    78,   167,   120,    29,   162,
      -1,    78,   167,   120,   163,   122,    29,    -1,    78,    23,
     120,   121,    29,    -1,    78,    23,   120,    11,    -1,    78,
      11,   120,    23,    -1,    78,   167,   120,    23,    -1,    82,
      -1,    83,    24,    -1,    88,   166,    -1,    88,    17,    -1,
      89,   166,    -1,    89,    17,    -1,    90,    -1,    91,    -1,
      92,    -1,    93,     3,   120,    29,    -1,    94,     3,   120,
      29,    -1,    95,    11,   120,    29,    -1,    95,    12,   120,
      29,    -1,    96,     3,   120,    29,    -1,    97,     3,   120,
      29,    -1,    98,     3,   120,    29,    -1,    99,    11,   120,
      29,    -1,   100,    25,    -1,   100,    26,    -1,   100,    27,
      -1,   100,    28,    -1,   102,     3,   120,    29,    -1,   102,
       5,   120,    29,    -1,   102,     6,   120,    29,    -1,   103,
      11,   120,    29,    -1,   103,    12,   120,    29,    -1,   104,
       3,   120,    29,    -1,   105,    11,   120,    29,    -1,   106,
      -1,   107,    -1,   108,    -1,   109,    -1,   110,    -1,   111,
      -1,   112,    -1,   116,     3,   120,   165,    -1,   116,     3,
     120,   163,   122,    29,    -1,   116,     3,   120,   163,   123,
      13,   124,    -1,   116,     3,   120,   163,   123,    13,   125,
      29,   124,    -1,   116,     3,   120,   163,   123,    14,   124,
      -1,   116,     3,   120,   163,   123,    14,   125,    29,   124,
      -1,   116,     3,   120,   163,   123,    14,   125,     5,   124,
      -1,   116,     3,   120,   163,   123,    14,   125,     6,   124,
      -1,   116,     3,   120,    29,    -1,   117,    11,   120,   167,
      -1,    -1,    19,   128,    -1,     4,    -1,     3,    -1,     6,
      -1,     5,    -1,     8,    -1,     7,    -1,    10,    -1,     9,
      -1,     4,    -1,     6,    -1,     5,    -1,     8,    -1,     7,
      -1,    10,    -1,     9,    -1,    11,    -1,    12,    -1,    13,
      -1,    14,    -1,    12,    -1,    13,    -1,    14,    -1,    15,
      -1,    16,    -1,    17,    -1,    18,    -1,    19,    -1,    20,
      -1,    21,    -1,    39,    -1,    40,    -1,   113,    -1,   114,
      -1,    60,    -1,    42,    -1,    86,    -1,   118,    -1,    41,
      -1,   115,    -1,    63,    -1,    43,    -1,    87,    -1,   119,
      -1,    52,    -1,    44,    -1,    53,    -1,   101,    -1,    57,
      -1,    84,    -1,    58,    -1,    85,    -1,    59,    -1,    71,
      -1,    64,    -1,    72,    -1,    65,    -1,    76,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   182,   182,   203,   206,   206,   209,   212,   215,   218,
     218,   221,   224,   227,   230,   233,   238,   247,   250,   253,
     253,   256,   259,   262,   265,   273,   273,   276,   276,   285,
     285,   288,   288,   293,   296,   299,   302,   305,   308,   313,
     316,   325,   328,   333,   336,   339,   342,   345,   350,   353,
     358,   361,   364,   367,   370,   391,   394,   397,   406,   409,
     412,   417,   419,   421,   423,   426,   426,   429,   434,   436,
     441,   444,   447,   450,   453,   453,   456,   461,   466,   469,
     469,   471,   473,   475,   477,   482,   485,   485,   488,   491,
     496,   499,   504,   507,   510,   513,   516,   519,   522,   527,
     535,   537,   540,   547,   547,   556,   559,   562,   565,   568,
     568,   577,   580,   583,   586,   589,   589,   598,   598,   598,
     601,   604,   611,   611,   611,   618,   621,   624,   627,   630,
     633,   636,   639,   642,   645,   648,   651,   654,   657,   660,
     663,   666,   669,   672,   675,   678,   681,   684,   687,   690,
     693,   696,   696,   699,   699,   702,   702,   705,   705,   708,
     708,   711,   711,   716,   725,   728,   731,   734,   743,   746,
     749,   754,   759,   762,   765,   765,   774,   774,   783,   783,
     792,   795,   798,   801,   804,   807,   810,   813,   816,   819,
     822,   825,   828,   831,   834,   837,   840,   843,   846,   849,
     852,   852,   855,   855,   858,   858,   861,   861,   864,   864,
     867,   870,   873,   876,   879,   884,   889,   894,   897,   900,
     903,   908,   911,   914,   919,   924,   929,   934,   939,   944,
     951,   956,   963,   966,   969,   972,   977,   982,   987,   992,
     997,  1004,  1009,  1016,  1019,  1022,  1025,  1028,  1031,  1036,
    1041,  1048,  1051,  1054,  1057,  1060,  1063,  1066,  1069,  1080,
    1089,  1090,  1094,  1095,  1096,  1097,  1098,  1099,  1100,  1101,
    1104,  1105,  1106,  1107,  1108,  1109,  1110,  1113,  1114,  1115,
    1116,  1119,  1120,  1121,  1124,  1125,  1126,  1127,  1128,  1129,
    1130,  1136,  1137,  1138,  1139,  1140,  1141,  1142,  1143,  1146,
    1147,  1148,  1151,  1152,  1153,  1156,  1157,  1158,  1161,  1162,
    1165,  1166,  1169,  1170,  1173,  1174,  1177,  1178,  1181
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "A", "X", "B", "C", "D", "E", "H", "L",
  "AX", "BC", "DE", "HL", "SPL", "SPH", "PSW", "CS", "ES", "PMC", "MEM",
  "FLAG", "SP", "CY", "RB0", "RB1", "RB2", "RB3", "EXPR", "UNKNOWN_OPCODE",
  "IS_OPCODE", "DOT_S", "DOT_B", "DOT_W", "DOT_L", "DOT_A", "DOT_UB",
  "DOT_UW", "ADD", "ADDC", "ADDW", "AND_", "AND1", "BF", "BH", "BNC",
  "BNH", "BNZ", "BR", "BRK", "BRK1", "BT", "BTCLR", "BZ", "CALL", "CALLT",
  "CLR1", "CLRB", "CLRW", "CMP", "CMP0", "CMPS", "CMPW", "DEC", "DECW",
  "DI", "DIVHU", "DIVWU", "EI", "HALT", "INC", "INCW", "MACH", "MACHU",
  "MOV", "MOV1", "MOVS", "MOVW", "MULH", "MULHU", "MULU", "NOP", "NOT1",
  "ONEB", "ONEW", "OR", "OR1", "POP", "PUSH", "RET", "RETI", "RETB", "ROL",
  "ROLC", "ROLWC", "ROR", "RORC", "SAR", "SARW", "SEL", "SET1", "SHL",
  "SHLW", "SHR", "SHRW", "SKC", "SKH", "SKNC", "SKNH", "SKNZ", "SKZ",
  "STOP", "SUB", "SUBC", "SUBW", "XCH", "XCHW", "XOR", "XOR1", "','",
  "'#'", "'!'", "'['", "']'", "'+'", "'.'", "'$'", "':'", "$accept",
  "statement", "@1", "@2", "@3", "@4", "@5", "@6", "@7", "@8", "@9", "@10",
  "@11", "@12", "@13", "@14", "@15", "@16", "@17", "@18", "@19", "@20",
  "@21", "@22", "@23", "@24", "@25", "@26", "@27", "@28", "@29", "@30",
  "@31", "@32", "opt_es", "regb", "regb_na", "regw", "regw_na", "sfr",
  "addsub", "addsubw", "andor1", "bt_bf", "setclr1", "oneclrb", "oneclrw",
  "incdec", "incdecw", "mov1", 0
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
     365,   366,   367,   368,   369,   370,   371,   372,   373,   374,
      44,    35,    33,    91,    93,    43,    46,    36,    58
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,   129,   130,   130,   131,   130,   130,   130,   130,   132,
     130,   130,   130,   130,   130,   130,   130,   130,   130,   133,
     130,   130,   130,   130,   130,   134,   130,   135,   130,   136,
     130,   137,   130,   130,   130,   130,   130,   130,   130,   130,
     130,   130,   130,   130,   130,   130,   130,   130,   130,   130,
     130,   130,   130,   130,   130,   130,   130,   130,   130,   130,
     130,   130,   130,   130,   130,   138,   130,   130,   130,   130,
     130,   130,   130,   130,   139,   130,   130,   130,   130,   140,
     130,   130,   130,   130,   130,   130,   141,   130,   130,   130,
     130,   130,   130,   130,   130,   130,   130,   130,   130,   130,
     130,   130,   130,   142,   130,   130,   130,   130,   130,   143,
     130,   130,   130,   130,   130,   144,   130,   145,   146,   130,
     130,   130,   147,   148,   130,   130,   130,   130,   130,   130,
     130,   130,   130,   130,   130,   130,   130,   130,   130,   130,
     130,   130,   130,   130,   130,   130,   130,   130,   130,   130,
     130,   149,   130,   150,   130,   151,   130,   152,   130,   153,
     130,   154,   130,   130,   130,   130,   130,   130,   130,   130,
     130,   130,   130,   130,   155,   130,   156,   130,   157,   130,
     130,   130,   130,   130,   130,   130,   130,   130,   130,   130,
     130,   130,   130,   130,   130,   130,   130,   130,   130,   130,
     158,   130,   159,   130,   160,   130,   161,   130,   162,   130,
     130,   130,   130,   130,   130,   130,   130,   130,   130,   130,
     130,   130,   130,   130,   130,   130,   130,   130,   130,   130,
     130,   130,   130,   130,   130,   130,   130,   130,   130,   130,
     130,   130,   130,   130,   130,   130,   130,   130,   130,   130,
     130,   130,   130,   130,   130,   130,   130,   130,   130,   130,
     163,   163,   164,   164,   164,   164,   164,   164,   164,   164,
     165,   165,   165,   165,   165,   165,   165,   166,   166,   166,
     166,   167,   167,   167,   168,   168,   168,   168,   168,   168,
     168,   169,   169,   169,   169,   169,   169,   169,   169,   170,
     170,   170,   171,   171,   171,   172,   172,   172,   173,   173,
     174,   174,   175,   175,   176,   176,   177,   177,   178
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     5,     0,     6,     4,     4,     4,     0,
       5,     6,     7,     9,     9,     9,     7,     5,     4,     0,
       5,     6,     9,     7,     5,     0,     7,     0,     7,     0,
       7,     0,    10,     3,     3,     3,     3,     3,     3,     7,
       7,     7,    10,     2,     3,     4,     3,     4,     1,     1,
       2,     4,     3,     4,     4,     2,     4,     4,     4,     6,
       7,     2,     2,     2,     2,     0,     3,     4,     2,     2,
       2,     2,     2,     2,     0,     3,     4,     9,     2,     0,
       3,     3,     5,     6,     8,     2,     0,     3,     4,     7,
       1,     1,     1,     1,     2,     1,     1,     1,     1,     1,
       5,     5,     5,     0,     7,     6,     8,     4,     4,     0,
       6,     6,     5,     7,     6,     0,     6,     0,     0,     7,
       4,     4,     0,     0,     7,     7,     7,    10,     9,     9,
       7,     7,    10,     9,     9,     9,     9,     9,     9,     9,
       8,     8,     9,     8,     8,     9,     8,     8,     7,     8,
       7,     0,    11,     0,     9,     0,    10,     0,     8,     0,
      10,     0,     8,     6,     6,     6,     9,     6,     6,     6,
       9,     9,     5,     5,     0,     7,     0,     6,     0,     6,
       4,     4,     6,     6,     7,     7,     9,     9,     7,     7,
       9,     9,     8,     8,     8,     8,     8,     7,     8,     7,
       0,    10,     0,     8,     0,    10,     0,     8,     0,     5,
       6,     5,     4,     4,     4,     1,     2,     2,     2,     2,
       2,     1,     1,     1,     4,     4,     4,     4,     4,     4,
       4,     4,     2,     2,     2,     2,     4,     4,     4,     4,
       4,     4,     4,     1,     1,     1,     1,     1,     1,     1,
       4,     6,     7,     9,     7,     9,     9,     9,     4,     4,
       0,     2,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
       0,     0,     2,   291,   292,   299,   296,   302,   306,     0,
       0,     0,     0,     0,    48,    49,   305,   307,     0,     0,
       0,   309,   311,   313,   295,   260,     0,   301,   315,   317,
      90,    95,    96,    91,    99,   314,   316,    98,    97,   260,
     318,   260,   260,    93,    92,     0,   215,     0,   310,   312,
     297,   303,     0,     0,   221,   222,   223,     0,     0,     0,
       0,     0,     0,     0,     0,   308,     0,     0,     0,     0,
     243,   244,   245,   246,   247,   248,   249,   293,   294,   300,
       0,     0,   298,   304,     0,   260,     0,     0,   260,   260,
     260,     0,     0,   260,   260,     0,     0,     0,     0,     0,
      43,     0,     0,     0,   277,   278,   279,   280,     0,     0,
      50,     0,    70,    71,    72,    73,     0,    74,     0,     0,
       0,   270,   272,   271,   274,   273,   276,   275,   284,   285,
     286,   287,   288,   289,   290,     0,     0,     0,     0,     0,
       0,   281,   282,   283,     0,     0,     0,    94,   216,   218,
     217,   220,   219,     0,     0,     0,     0,     0,     0,     0,
       0,   232,   233,   234,   235,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     1,     0,     4,     0,     0,     0,
       0,     0,     0,   288,     0,     0,     0,     0,    55,     0,
       0,     0,    61,    62,    63,    64,    65,     0,    68,    69,
     263,   262,   265,   264,   267,   266,   269,   268,     0,    79,
       0,     0,    78,    86,     0,    85,     0,     0,     0,     0,
       0,    33,    37,    34,    38,    36,    46,     0,    44,     0,
      35,    52,     0,     0,     0,   261,    75,     0,   260,   260,
     261,     0,     0,     0,   260,   260,     0,   260,     0,     0,
       0,     0,   260,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   260,     0,
     260,     0,     0,     0,   260,     0,   260,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    66,     0,     0,    80,
      81,     0,    87,     0,     0,     0,   260,     0,     0,     0,
      47,    45,    53,    51,    54,    76,     0,     0,     0,   108,
     120,     0,     0,     0,     0,     0,     0,     0,     0,   107,
       0,     0,     0,     0,   121,     0,   213,     0,     0,   180,
     212,     0,     0,     0,     0,     0,     0,     0,     0,   181,
     214,   208,     0,     0,   224,   225,   226,   227,   228,   229,
     230,   231,   236,   237,   238,   239,   240,   241,   242,   258,
       0,   250,   259,     6,     9,     0,     0,     7,     0,     0,
       8,    19,     0,     0,    18,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    58,    57,     0,     0,    56,    67,
       0,     0,     0,    88,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   100,   115,     0,     0,     0,   112,
       0,   109,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   101,   117,     0,   102,   122,     0,   172,
     176,     0,     0,   211,   178,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   209,   173,     0,
       0,     0,    10,     3,     0,     0,     0,     0,    20,    17,
       0,     0,    24,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    82,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   116,   111,     0,
       0,     0,     0,     0,   105,   110,   103,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   118,
     114,   123,     0,     0,   177,   182,     0,     0,     0,     0,
     179,   174,     0,     0,     0,   183,     0,     0,     0,     0,
       0,     0,     0,   210,   251,     0,     0,    11,     0,     5,
       0,    21,     0,    29,    27,     0,    25,     0,     0,     0,
       0,    59,     0,     0,    83,     0,   168,   164,   163,     0,
     165,   167,     0,   169,     0,     0,     0,     0,   148,   125,
       0,   130,     0,   157,     0,   113,     0,   104,     0,     0,
       0,   150,     0,   126,     0,   131,     0,     0,     0,   161,
       0,     0,   119,   124,     0,     0,     0,     0,   197,   184,
       0,   188,     0,   202,     0,   175,     0,     0,     0,   199,
     185,     0,   189,     0,   206,     0,   252,     0,   254,     0,
      12,     0,    16,    23,     0,    30,    28,     0,    26,    41,
      40,     0,    39,    60,     0,    89,     0,     0,     0,   140,
     143,   147,     0,     0,     0,     0,   158,     0,   106,   141,
       0,   144,     0,   149,     0,   146,     0,     0,     0,     0,
     162,   153,     0,     0,   192,   194,   196,     0,     0,   203,
       0,   193,   195,   198,     0,     0,   207,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    84,     0,
       0,    77,   128,   135,   137,   133,   155,   139,   142,   145,
     129,     0,   136,   138,   134,     0,   154,   159,     0,   171,
     186,   190,   200,   187,   191,   204,   253,   256,   257,   255,
      14,    15,    13,    22,    31,     0,   166,   170,   156,   127,
     132,   160,   151,   201,   205,    32,    42,   152
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    84,   271,   452,   458,   628,   626,   625,   735,   286,
     236,   289,   292,   577,   495,   487,   509,   592,   511,   593,
     737,   706,   728,   646,   731,   660,   605,   514,   520,   733,
     669,   734,   676,   447,   118,   212,   137,   110,   146,   138,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -206
static const yytype_int16 yypact[] =
{
     220,   -44,  -206,  -206,  -206,  -206,  -206,  -206,  -206,   -34,
     -24,    17,    62,    41,  -206,  -206,  -206,  -206,    64,    93,
      75,  -206,  -206,  -206,  -206,   161,   203,  -206,  -206,  -206,
    -206,  -206,  -206,  -206,  -206,  -206,  -206,  -206,  -206,    53,
    -206,   183,   228,  -206,  -206,   224,  -206,   206,  -206,  -206,
    -206,  -206,   403,   422,  -206,  -206,  -206,   235,   251,    97,
     253,   345,   356,   225,   336,  -206,    38,   150,   382,   247,
    -206,  -206,  -206,  -206,  -206,  -206,  -206,  -206,  -206,  -206,
     401,   383,  -206,  -206,   246,   337,   149,   389,   390,   352,
     346,   167,    10,   198,   371,   394,   408,   413,   429,   445,
    -206,    24,    63,   466,  -206,  -206,  -206,  -206,    81,   296,
    -206,   467,  -206,  -206,  -206,  -206,   304,  -206,   354,   379,
     380,  -206,  -206,  -206,  -206,  -206,  -206,  -206,  -206,  -206,
    -206,  -206,   374,  -206,  -206,   472,    11,   384,   385,   386,
     387,  -206,  -206,  -206,   388,    36,   392,  -206,  -206,  -206,
    -206,  -206,  -206,   393,   395,   396,   397,   398,   399,   400,
     402,  -206,  -206,  -206,  -206,   404,   405,   406,   407,   410,
     411,   412,   414,   415,  -206,   416,  -206,   381,   417,   418,
     419,   420,   421,   304,   423,   425,   424,   426,  -206,   427,
       2,   430,  -206,  -206,  -206,  -206,  -206,   432,  -206,  -206,
    -206,  -206,  -206,  -206,  -206,  -206,  -206,  -206,   378,  -206,
     481,   497,  -206,  -206,   121,  -206,   431,   435,   433,   437,
     436,  -206,  -206,  -206,  -206,  -206,  -206,   485,  -206,   492,
    -206,  -206,   494,   499,   434,  -206,  -206,   500,   183,    80,
     439,   443,    96,   169,    29,    72,   519,   124,    55,    98,
     512,   211,   112,   513,   514,   515,   516,   517,   522,   535,
     536,   537,   538,   539,   540,   541,   542,   543,   446,   157,
      18,   453,   545,   572,   101,   456,   409,   547,   549,   565,
     551,   552,   553,   554,   570,   556,  -206,   557,   130,  -206,
    -206,   462,  -206,   559,   575,   561,   428,   562,   578,   564,
    -206,  -206,  -206,  -206,  -206,  -206,   471,   566,    65,  -206,
    -206,   567,     5,     6,   243,   473,   233,   255,   258,  -206,
     569,    82,   571,   573,  -206,   474,  -206,   574,    73,  -206,
    -206,   576,    56,   348,   484,   477,   274,   277,   297,  -206,
    -206,  -206,   577,   486,  -206,  -206,  -206,  -206,  -206,  -206,
    -206,  -206,  -206,  -206,  -206,  -206,  -206,  -206,  -206,  -206,
     318,  -206,  -206,  -206,  -206,   580,   344,  -206,   489,   487,
    -206,  -206,   582,   349,  -206,   583,   488,   490,   495,   491,
     493,   501,   496,   502,  -206,  -206,   498,   503,  -206,  -206,
     586,   605,   594,  -206,   504,   505,   506,   507,   508,   509,
     510,   518,   520,   612,  -206,   511,   599,   447,   521,  -206,
     607,  -206,   608,   523,   524,   525,   526,   530,   609,   531,
     111,   532,   610,  -206,  -206,   614,  -206,  -206,   615,  -206,
     533,   616,   450,  -206,  -206,   624,   534,   544,   546,   643,
     555,   558,   626,   560,   628,   563,   630,  -206,  -206,   631,
     632,   455,  -206,  -206,   633,   649,   635,   548,  -206,  -206,
     636,   652,  -206,   638,   642,   658,   644,   550,   568,   579,
     581,   645,   584,  -206,   587,   585,   647,   655,   653,   656,
     667,   657,   660,   588,   663,   590,   372,  -206,  -206,   589,
     353,   355,   357,     7,  -206,  -206,  -206,   591,   596,   597,
       8,   685,   595,   686,   598,   600,   601,    27,   602,  -206,
    -206,  -206,   603,   391,  -206,  -206,   604,   359,   361,   363,
    -206,  -206,   611,   613,   617,  -206,   679,   680,   606,   681,
     618,   682,   619,  -206,  -206,   365,   367,  -206,   369,  -206,
     665,  -206,   373,  -206,  -206,   620,  -206,   668,   669,   670,
     671,  -206,   672,   673,  -206,   621,  -206,  -206,  -206,   622,
    -206,  -206,   674,  -206,   675,   623,   625,   627,  -206,  -206,
     677,  -206,   113,  -206,   678,  -206,   689,  -206,    28,    30,
      31,  -206,   691,  -206,   634,  -206,   637,   639,   640,  -206,
     692,   641,  -206,  -206,   646,   629,   648,   650,  -206,  -206,
     694,  -206,   700,  -206,   703,  -206,   723,   724,   725,  -206,
    -206,   651,  -206,   659,  -206,   661,  -206,   709,  -206,   116,
    -206,   168,  -206,  -206,   710,  -206,  -206,   654,  -206,  -206,
    -206,   662,  -206,  -206,   664,  -206,   666,   676,   683,  -206,
    -206,  -206,   684,   687,   688,   690,  -206,   693,  -206,  -206,
     711,  -206,   712,  -206,   719,  -206,    32,   747,   749,    33,
    -206,  -206,    35,   751,  -206,  -206,  -206,   695,   696,  -206,
     697,  -206,  -206,  -206,   745,   752,  -206,   753,   698,   699,
     701,   702,   704,   705,   706,   707,   729,   708,  -206,   733,
     741,  -206,  -206,  -206,  -206,  -206,  -206,  -206,  -206,  -206,
    -206,   738,  -206,  -206,  -206,   739,  -206,  -206,   740,  -206,
    -206,  -206,  -206,  -206,  -206,  -206,  -206,  -206,  -206,  -206,
    -206,  -206,  -206,  -206,  -206,   744,  -206,  -206,  -206,  -206,
    -206,  -206,  -206,  -206,  -206,  -206,  -206,  -206
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -206,  -206,  -206,  -206,  -206,  -206,  -206,  -206,  -206,  -206,
    -206,  -206,  -206,  -206,  -206,  -206,  -206,  -206,  -206,  -206,
    -206,  -206,  -206,  -206,  -206,  -206,  -206,  -206,  -206,  -206,
    -206,  -206,  -206,  -206,   -39,   451,   -84,   -48,  -205,   -82,
    -206,  -206,  -206,  -206,  -206,  -206,  -206,  -206,  -206,  -206
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const yytype_uint16 yytable[] =
{
     136,   178,   139,   145,   150,   152,   186,   191,   409,   411,
     575,   581,   220,   200,   201,   202,   203,   204,   205,   206,
     207,   363,   121,   122,   123,   124,   125,   126,   127,   208,
     589,   649,   319,   651,   653,   700,   704,   116,   707,   209,
     242,   165,   329,   166,   167,   215,   177,   364,   116,   185,
     190,   197,   100,   226,   214,   219,   120,   121,   122,   123,
     124,   125,   126,   127,   362,   249,   330,   434,   128,   129,
     130,   131,   132,   133,   134,   200,   201,   202,   203,   204,
     205,   206,   207,    95,   121,   122,   123,   124,   125,   126,
     127,   116,   228,    96,   405,   128,   129,   130,   131,   183,
     133,   134,   430,    97,   104,   105,   106,   107,   155,   156,
     231,   424,   104,   105,   106,   107,   504,   505,   643,   644,
     116,   679,   680,   339,   283,   284,   410,   412,   576,   582,
     371,   116,   210,   211,   243,   340,   141,   142,   143,   365,
     506,   341,   645,   116,    98,   681,   227,   326,   590,   650,
     320,   652,   654,   701,   705,   309,   708,   310,   250,   251,
     179,   168,   169,   101,   112,   113,   114,   115,   102,   141,
     142,   143,   180,   682,   683,   135,   331,   435,   198,   199,
     116,   315,   316,   317,   361,   229,   367,   406,   407,    99,
     117,   103,   318,   322,   379,   431,   432,   684,   111,   306,
     308,   307,   116,   232,   425,   321,   323,   119,   328,   104,
     105,   106,   107,   343,   399,   108,   313,   116,   332,   314,
     109,   333,   372,   335,   336,   337,   374,   213,   147,   360,
     148,   366,     1,   342,   338,   373,   160,   378,   153,   140,
     141,   142,   143,   293,   294,   327,   174,   116,   413,   414,
       2,   144,   390,   391,   154,   415,   157,   398,   171,     3,
       4,     5,     6,     7,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
     175,   121,   122,   123,   124,   125,   126,   127,   158,   192,
     193,   194,   195,   436,   437,   187,   116,   417,   418,   159,
     438,   161,   162,   163,   164,   116,   176,   128,   129,   130,
     131,   183,   133,   134,   216,   196,   188,   565,   566,   419,
     420,   189,   421,   422,   567,   170,   128,   129,   130,   131,
     183,   133,   134,   182,   173,   217,   595,   596,   441,   442,
     218,   443,   444,   597,   172,   128,   129,   130,   131,   183,
     133,   134,   376,   181,   104,   105,   106,   107,   233,   184,
     149,   445,   446,   221,   128,   129,   130,   131,   183,   133,
     134,   396,   235,   104,   105,   106,   107,   222,   377,   151,
     450,   451,   223,   128,   129,   130,   131,   183,   133,   134,
     121,   122,   123,   124,   125,   126,   127,   397,   224,   489,
     490,   491,   516,   517,   518,   116,   454,   455,   535,   536,
     492,   460,   461,   519,   225,   359,   237,   569,   570,   571,
     572,   573,   574,   599,   600,   601,   602,   603,   604,   616,
     617,   618,   619,   620,   621,   230,   234,   623,   624,   238,
     239,   241,   240,   272,   244,   245,   288,   247,   248,   246,
     290,   291,   252,   253,   300,   254,   255,   256,   257,   258,
     259,   301,   260,   302,   261,   262,   263,   264,   303,   305,
     265,   266,   267,   325,   268,   269,   270,   273,   274,   275,
     276,   334,   344,   345,   346,   347,   348,   277,   279,   278,
     280,   349,   281,   282,   287,   296,   285,   295,   304,   297,
     298,   311,   299,   312,   350,   351,   352,   353,   354,   355,
     356,   357,   358,   368,   369,   370,   380,   375,   381,   382,
     383,   384,   385,   386,   387,   388,   389,   392,   393,   394,
     395,   400,   401,   402,   403,   404,   408,   416,   423,   428,
     426,   440,   427,   429,   439,   433,   448,   457,   449,   453,
     456,   459,   462,   467,   463,   473,   464,   466,   465,   474,
     469,   468,   470,   475,   471,   477,   485,   472,   488,   476,
     482,   480,   478,   479,   486,   481,   494,   496,   502,   508,
     484,   493,   483,   510,   512,   515,   500,   497,   498,   499,
     501,   503,   507,   521,   525,   528,   513,   530,   522,   532,
     533,   534,   537,   538,   539,   541,   542,   543,   523,   540,
     524,   544,   545,   546,   551,   526,   555,   547,   527,   556,
     529,   559,   557,   531,   561,   558,   560,   563,   583,   585,
     609,   610,   612,   614,   622,   548,   324,   629,   630,   631,
     632,   633,   634,   637,   638,   549,   642,   647,   550,   554,
     552,   578,   553,   568,   562,   564,   579,   580,   648,   584,
     655,   661,   586,   667,   587,   588,   591,   594,   598,   668,
     611,   606,   670,   607,   671,   672,   673,   608,   678,   685,
     697,   698,   613,   615,   627,   635,   636,   639,   699,   640,
     702,   641,   703,   664,   656,   709,   713,   657,   724,   658,
     659,   662,   726,   714,   715,   727,   663,   729,   730,   732,
       0,   674,   665,   736,   666,     0,     0,     0,     0,   675,
     686,   677,   687,     0,     0,     0,     0,     0,   688,     0,
       0,     0,   689,     0,     0,     0,   690,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   691,   692,     0,
       0,   693,   694,     0,   695,     0,     0,   696,     0,   710,
     711,   712,   716,   717,     0,   718,   719,     0,   720,   721,
     722,   723,     0,     0,     0,   725
};

static const yytype_int16 yycheck[] =
{
      39,    85,    41,    42,    52,    53,    88,    89,     3,     3,
       3,     3,    94,     3,     4,     5,     6,     7,     8,     9,
      10,     3,     4,     5,     6,     7,     8,     9,    10,    19,
       3,     3,     3,     3,     3,     3,     3,    19,     3,    29,
      29,     3,   247,     5,     6,    93,    85,    29,    19,    88,
      89,    90,    11,    29,    93,    94,     3,     4,     5,     6,
       7,     8,     9,    10,   269,    29,    11,    11,    15,    16,
      17,    18,    19,    20,    21,     3,     4,     5,     6,     7,
       8,     9,    10,   127,     4,     5,     6,     7,     8,     9,
      10,    19,    29,   127,    29,    15,    16,    17,    18,    19,
      20,    21,    29,   127,    11,    12,    13,    14,    11,    12,
      29,    29,    11,    12,    13,    14,     5,     6,     5,     6,
      19,     5,     6,    11,   122,   123,   121,   121,   121,   121,
      29,    19,   122,   123,   123,    23,    12,    13,    14,   121,
      29,    29,    29,    19,   127,    29,   122,    23,   121,   121,
     121,   121,   121,   121,   121,   239,   121,   239,   122,   123,
      11,    11,    12,   122,     3,     4,     5,     6,   127,    12,
      13,    14,    23,     5,     6,   122,   121,   121,    11,    12,
      19,    12,    13,    14,   268,   122,   270,   122,   123,   127,
      29,   127,    23,   121,   276,   122,   123,    29,   123,   238,
     239,   121,    19,   122,   122,   244,   245,     4,   247,    11,
      12,    13,    14,   252,   296,   122,   120,    19,   120,   123,
     127,   123,   121,    12,    13,    14,   274,    29,     4,   268,
      24,   270,    12,   121,    23,   274,    11,   276,     3,    11,
      12,    13,    14,   122,   123,   121,     0,    19,     5,     6,
      30,    23,   122,   123,     3,    12,     3,   296,    11,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    92,    93,    94,    95,    96,    97,    98,    99,
     100,   101,   102,   103,   104,   105,   106,   107,   108,   109,
     110,   111,   112,   113,   114,   115,   116,   117,   118,   119,
       3,     4,     5,     6,     7,     8,     9,    10,     3,     3,
       4,     5,     6,     5,     6,     3,    19,   124,   125,     3,
      12,    25,    26,    27,    28,    19,    29,    15,    16,    17,
      18,    19,    20,    21,     3,    29,    24,     5,     6,   124,
     125,    29,   124,   125,    12,     3,    15,    16,    17,    18,
      19,    20,    21,     3,    11,    24,     5,     6,   124,   125,
      29,   124,   125,    12,     3,    15,    16,    17,    18,    19,
      20,    21,     3,    24,    11,    12,    13,    14,   122,    29,
      17,   124,   125,    29,    15,    16,    17,    18,    19,    20,
      21,     3,   128,    11,    12,    13,    14,    29,    29,    17,
     122,   123,    29,    15,    16,    17,    18,    19,    20,    21,
       4,     5,     6,     7,     8,     9,    10,    29,    29,    12,
      13,    14,    12,    13,    14,    19,   122,   123,    13,    14,
      23,   122,   123,    23,    29,    29,   122,   124,   125,   124,
     125,   124,   125,   124,   125,   124,   125,   124,   125,   124,
     125,   124,   125,   124,   125,    29,    29,   124,   125,   120,
     120,    29,   128,   122,   120,   120,   128,   120,   120,   123,
      29,    14,   120,   120,    29,   120,   120,   120,   120,   120,
     120,    29,   120,    29,   120,   120,   120,   120,    29,    29,
     120,   120,   120,    14,   120,   120,   120,   120,   120,   120,
     120,    29,    29,    29,    29,    29,    29,   126,   123,   126,
     126,    29,   126,   126,   122,   120,   126,   126,   124,   126,
     123,   122,   126,   120,    29,    29,    29,    29,    29,    29,
      29,    29,    29,   120,    29,     3,    29,   121,    29,    14,
      29,    29,    29,    29,    14,    29,    29,   125,    29,    14,
      29,    29,    14,    29,   123,    29,    29,   124,    29,   125,
      29,   124,    29,    29,   120,    29,    29,   120,   122,    29,
     121,    29,    29,   120,   126,    29,   126,   126,   123,    14,
     124,   120,   120,    29,   126,   120,    14,   124,    29,   125,
     120,   123,   126,   126,   123,   126,    29,    29,    29,    29,
     120,   120,   124,    29,    29,    29,   120,   124,   124,   124,
     120,   120,   120,    29,    11,    29,   123,    29,   124,    29,
      29,    29,    29,    14,    29,    29,    14,    29,   124,   121,
     124,    29,    14,    29,    29,   120,    29,   127,   120,    24,
     120,    14,    29,   120,    24,    29,    29,    24,     3,     3,
      11,    11,    11,    11,    29,   127,   245,    29,    29,    29,
      29,    29,    29,    29,    29,   126,    29,    29,   127,   124,
     126,   120,   125,   124,   126,   125,   120,   120,    29,   124,
      29,    29,   124,    29,   124,   124,   124,   124,   124,    29,
     124,   120,    29,   120,    11,    11,    11,   120,    29,    29,
      29,    29,   124,   124,   124,   124,   124,   124,    29,   124,
       3,   124,     3,   124,   120,     4,    11,   120,    29,   120,
     120,   120,    29,    11,    11,    24,   120,    29,    29,    29,
      -1,   120,   124,    29,   124,    -1,    -1,    -1,    -1,   120,
     126,   120,   120,    -1,    -1,    -1,    -1,    -1,   124,    -1,
      -1,    -1,   126,    -1,    -1,    -1,   120,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   124,   124,    -1,
      -1,   124,   124,    -1,   124,    -1,    -1,   124,    -1,   124,
     124,   124,   124,   124,    -1,   124,   124,    -1,   124,   124,
     124,   124,    -1,    -1,    -1,   127
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,    12,    30,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   110,   111,   112,   113,   114,   115,
     116,   117,   118,   119,   130,   169,   170,   171,   172,   173,
     174,   175,   176,   177,   178,   127,   127,   127,   127,   127,
      11,   122,   127,   127,    11,    12,    13,    14,   122,   127,
     166,   123,     3,     4,     5,     6,    19,    29,   163,     4,
       3,     4,     5,     6,     7,     8,     9,    10,    15,    16,
      17,    18,    19,    20,    21,   122,   163,   165,   168,   163,
      11,    12,    13,    14,    23,   163,   167,     4,    24,    17,
     166,    17,   166,     3,     3,    11,    12,     3,     3,     3,
      11,    25,    26,    27,    28,     3,     5,     6,    11,    12,
       3,    11,     3,    11,     0,     3,    29,   163,   165,    11,
      23,    24,     3,    19,    29,   163,   168,     3,    24,    29,
     163,   168,     3,     4,     5,     6,    29,   163,    11,    12,
       3,     4,     5,     6,     7,     8,     9,    10,    19,    29,
     122,   123,   164,    29,   163,   166,     3,    24,    29,   163,
     168,    29,    29,    29,    29,    29,    29,   122,    29,   122,
      29,    29,   122,   122,    29,   128,   139,   122,   120,   120,
     128,    29,    29,   123,   120,   120,   123,   120,   120,    29,
     122,   123,   120,   120,   120,   120,   120,   120,   120,   120,
     120,   120,   120,   120,   120,   120,   120,   120,   120,   120,
     120,   131,   122,   120,   120,   120,   120,   126,   126,   123,
     126,   126,   126,   122,   123,   126,   138,   122,   128,   140,
      29,    14,   141,   122,   123,   126,   120,   126,   123,   126,
      29,    29,    29,    29,   124,    29,   163,   121,   163,   165,
     168,   122,   120,   120,   123,    12,    13,    14,    23,     3,
     121,   163,   121,   163,   164,    14,    23,   121,   163,   167,
      11,   121,   120,   123,    29,    12,    13,    14,    23,    11,
      23,    29,   121,   163,    29,    29,    29,    29,    29,    29,
      29,    29,    29,    29,    29,    29,    29,    29,    29,    29,
     163,   165,   167,     3,    29,   121,   163,   165,   120,    29,
       3,    29,   121,   163,   166,   121,     3,    29,   163,   168,
      29,    29,    14,    29,    29,    29,    29,    14,    29,    29,
     122,   123,   125,    29,    14,    29,     3,    29,   163,   168,
      29,    14,    29,   123,    29,    29,   122,   123,    29,     3,
     121,     3,   121,     5,     6,    12,   124,   124,   125,   124,
     125,   124,   125,    29,    29,   122,    29,    29,   125,    29,
      29,   122,   123,    29,    11,   121,     5,     6,    12,   120,
     124,   124,   125,   124,   125,   124,   125,   162,    29,   122,
     122,   123,   132,    29,   122,   123,   121,   120,   133,    29,
     122,   123,    29,   126,   126,   123,   126,   120,   120,   124,
     120,   126,   124,    29,    14,    29,   125,   120,   126,   126,
     123,   126,   120,   124,   120,    14,   123,   144,    29,    12,
      13,    14,    23,   120,    29,   143,    29,   124,   124,   124,
     120,   120,    29,   120,     5,     6,    29,   120,    29,   145,
      29,   147,    29,   123,   156,    29,    12,    13,    14,    23,
     157,    29,   124,   124,   124,    11,   120,   120,    29,   120,
      29,   120,    29,    29,    29,    13,    14,    29,    14,    29,
     121,    29,    14,    29,    29,    14,    29,   127,   127,   126,
     127,    29,   126,   125,   124,    29,    24,    29,    29,    14,
      29,    24,   126,    24,   125,     5,     6,    12,   124,   124,
     125,   124,   125,   124,   125,     3,   121,   142,   120,   120,
     120,     3,   121,     3,   124,     3,   124,   124,   124,     3,
     121,   124,   146,   148,   124,     5,     6,    12,   124,   124,
     125,   124,   125,   124,   125,   155,   120,   120,   120,    11,
      11,   124,    11,   124,    11,   124,   124,   125,   124,   125,
     124,   125,    29,   124,   125,   136,   135,   124,   134,    29,
      29,    29,    29,    29,    29,   124,   124,    29,    29,   124,
     124,   124,    29,     5,     6,    29,   152,    29,    29,     3,
     121,     3,   121,     3,   121,    29,   120,   120,   120,   120,
     154,    29,   120,   120,   124,   124,   124,    29,    29,   159,
      29,    11,    11,    11,   120,   120,   161,   120,    29,     5,
       6,    29,     5,     6,    29,    29,   126,   120,   124,   126,
     120,   124,   124,   124,   124,   124,   124,    29,    29,    29,
       3,   121,     3,     3,     3,   121,   150,     3,   121,     4,
     124,   124,   124,    11,    11,    11,   124,   124,   124,   124,
     124,   124,   124,   124,    29,   127,    29,    24,   151,    29,
      29,   153,    29,   158,   160,   137,    29,   149
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
#line 183 "rl78-parse.y"
    { as_bad (_("Unknown opcode: %s"), rl78_init_start); }
    break;

  case 3:
#line 204 "rl78-parse.y"
    { B1 (0x0c|(yyvsp[(1) - (5)].regno)); O1 ((yyvsp[(5) - (5)].exp)); }
    break;

  case 4:
#line 206 "rl78-parse.y"
    {SA((yyvsp[(2) - (2)].exp))}
    break;

  case 5:
#line 207 "rl78-parse.y"
    { B1 (0x0a|(yyvsp[(1) - (6)].regno)); O1 ((yyvsp[(2) - (6)].exp)); O1 ((yyvsp[(6) - (6)].exp)); }
    break;

  case 6:
#line 210 "rl78-parse.y"
    { B2 (0x61, 0x01|(yyvsp[(1) - (4)].regno)); }
    break;

  case 7:
#line 213 "rl78-parse.y"
    { B2 (0x61, 0x08|(yyvsp[(1) - (4)].regno)); F ((yyvsp[(4) - (4)].regno), 13, 3); }
    break;

  case 8:
#line 216 "rl78-parse.y"
    { B2 (0x61, 0x00|(yyvsp[(1) - (4)].regno)); F ((yyvsp[(2) - (4)].regno), 13, 3); }
    break;

  case 9:
#line 218 "rl78-parse.y"
    {SA((yyvsp[(4) - (4)].exp))}
    break;

  case 10:
#line 219 "rl78-parse.y"
    { B1 (0x0b|(yyvsp[(1) - (5)].regno)); O1 ((yyvsp[(4) - (5)].exp)); }
    break;

  case 11:
#line 222 "rl78-parse.y"
    { B1 (0x0f|(yyvsp[(1) - (6)].regno)); O2 ((yyvsp[(6) - (6)].exp)); rl78_linkrelax_addr16 (); }
    break;

  case 12:
#line 225 "rl78-parse.y"
    { B1 (0x0d|(yyvsp[(1) - (7)].regno)); }
    break;

  case 13:
#line 228 "rl78-parse.y"
    { B1 (0x0e|(yyvsp[(1) - (9)].regno)); O1 ((yyvsp[(8) - (9)].exp)); }
    break;

  case 14:
#line 231 "rl78-parse.y"
    { B2 (0x61, 0x80|(yyvsp[(1) - (9)].regno)); }
    break;

  case 15:
#line 234 "rl78-parse.y"
    { B2 (0x61, 0x82|(yyvsp[(1) - (9)].regno)); }
    break;

  case 16:
#line 239 "rl78-parse.y"
    { if ((yyvsp[(1) - (7)].regno) != 0x40)
	      { rl78_error ("Only CMP takes these operands"); }
	    else
	      { B1 (0x00|(yyvsp[(1) - (7)].regno)); O2 ((yyvsp[(4) - (7)].exp)); O1 ((yyvsp[(7) - (7)].exp)); rl78_linkrelax_addr16 (); }
	  }
    break;

  case 17:
#line 248 "rl78-parse.y"
    { B1 (0x04|(yyvsp[(1) - (5)].regno)); O2 ((yyvsp[(5) - (5)].exp)); }
    break;

  case 18:
#line 251 "rl78-parse.y"
    { B1 (0x01|(yyvsp[(1) - (4)].regno)); F ((yyvsp[(4) - (4)].regno), 5, 2); }
    break;

  case 19:
#line 253 "rl78-parse.y"
    {SA((yyvsp[(4) - (4)].exp))}
    break;

  case 20:
#line 254 "rl78-parse.y"
    { B1 (0x06|(yyvsp[(1) - (5)].regno)); O1 ((yyvsp[(4) - (5)].exp)); }
    break;

  case 21:
#line 257 "rl78-parse.y"
    { B1 (0x02|(yyvsp[(1) - (6)].regno)); O2 ((yyvsp[(6) - (6)].exp)); rl78_linkrelax_addr16 (); }
    break;

  case 22:
#line 260 "rl78-parse.y"
    { B2 (0x61, 0x09|(yyvsp[(1) - (9)].regno)); O1 ((yyvsp[(8) - (9)].exp)); }
    break;

  case 23:
#line 263 "rl78-parse.y"
    { B4 (0x61, 0x09|(yyvsp[(1) - (7)].regno), 0, 0); }
    break;

  case 24:
#line 266 "rl78-parse.y"
    { B1 ((yyvsp[(1) - (5)].regno) ? 0x20 : 0x10); O1 ((yyvsp[(5) - (5)].exp));
	    if ((yyvsp[(1) - (5)].regno) == 0x40)
	      rl78_error ("CMPW SP,#imm not allowed");
	  }
    break;

  case 25:
#line 273 "rl78-parse.y"
    {Bit((yyvsp[(6) - (6)].exp))}
    break;

  case 26:
#line 274 "rl78-parse.y"
    { B3 (0x71, 0x08|(yyvsp[(1) - (7)].regno), (yyvsp[(4) - (7)].regno)); FE ((yyvsp[(6) - (7)].exp), 9, 3); }
    break;

  case 27:
#line 276 "rl78-parse.y"
    {Bit((yyvsp[(6) - (6)].exp))}
    break;

  case 28:
#line 277 "rl78-parse.y"
    { if (expr_is_sfr ((yyvsp[(4) - (7)].exp)))
	      { B2 (0x71, 0x08|(yyvsp[(1) - (7)].regno)); FE ((yyvsp[(6) - (7)].exp), 9, 3); O1 ((yyvsp[(4) - (7)].exp)); }
	    else if (expr_is_saddr ((yyvsp[(4) - (7)].exp)))
	      { B2 (0x71, 0x00|(yyvsp[(1) - (7)].regno)); FE ((yyvsp[(6) - (7)].exp), 9, 3); O1 ((yyvsp[(4) - (7)].exp)); }
	    else
	      NOT_SFR_OR_SADDR;
	  }
    break;

  case 29:
#line 285 "rl78-parse.y"
    {Bit((yyvsp[(6) - (6)].exp))}
    break;

  case 30:
#line 286 "rl78-parse.y"
    { B2 (0x71, 0x88|(yyvsp[(1) - (7)].regno));  FE ((yyvsp[(6) - (7)].exp), 9, 3); }
    break;

  case 31:
#line 288 "rl78-parse.y"
    {Bit((yyvsp[(9) - (9)].exp))}
    break;

  case 32:
#line 289 "rl78-parse.y"
    { B2 (0x71, 0x80|(yyvsp[(1) - (10)].regno));  FE ((yyvsp[(9) - (10)].exp), 9, 3); }
    break;

  case 33:
#line 294 "rl78-parse.y"
    { B1 (0xdc); PC1 ((yyvsp[(3) - (3)].exp)); }
    break;

  case 34:
#line 297 "rl78-parse.y"
    { B1 (0xde); PC1 ((yyvsp[(3) - (3)].exp)); }
    break;

  case 35:
#line 300 "rl78-parse.y"
    { B1 (0xdd); PC1 ((yyvsp[(3) - (3)].exp)); }
    break;

  case 36:
#line 303 "rl78-parse.y"
    { B1 (0xdf); PC1 ((yyvsp[(3) - (3)].exp)); }
    break;

  case 37:
#line 306 "rl78-parse.y"
    { B2 (0x61, 0xc3); PC1 ((yyvsp[(3) - (3)].exp)); }
    break;

  case 38:
#line 309 "rl78-parse.y"
    { B2 (0x61, 0xd3); PC1 ((yyvsp[(3) - (3)].exp)); }
    break;

  case 39:
#line 314 "rl78-parse.y"
    { B3 (0x31, 0x80|(yyvsp[(1) - (7)].regno), (yyvsp[(2) - (7)].regno)); FE ((yyvsp[(4) - (7)].exp), 9, 3); PC1 ((yyvsp[(7) - (7)].exp)); }
    break;

  case 40:
#line 317 "rl78-parse.y"
    { if (expr_is_sfr ((yyvsp[(2) - (7)].exp)))
	      { B2 (0x31, 0x80|(yyvsp[(1) - (7)].regno)); FE ((yyvsp[(4) - (7)].exp), 9, 3); O1 ((yyvsp[(2) - (7)].exp)); PC1 ((yyvsp[(7) - (7)].exp)); }
	    else if (expr_is_saddr ((yyvsp[(2) - (7)].exp)))
	      { B2 (0x31, 0x00|(yyvsp[(1) - (7)].regno)); FE ((yyvsp[(4) - (7)].exp), 9, 3); O1 ((yyvsp[(2) - (7)].exp)); PC1 ((yyvsp[(7) - (7)].exp)); }
	    else
	      NOT_SFR_OR_SADDR;
	  }
    break;

  case 41:
#line 326 "rl78-parse.y"
    { B2 (0x31, 0x01|(yyvsp[(1) - (7)].regno)); FE ((yyvsp[(4) - (7)].exp), 9, 3); PC1 ((yyvsp[(7) - (7)].exp)); }
    break;

  case 42:
#line 329 "rl78-parse.y"
    { B2 (0x31, 0x81|(yyvsp[(1) - (10)].regno)); FE ((yyvsp[(7) - (10)].exp), 9, 3); PC1 ((yyvsp[(10) - (10)].exp)); }
    break;

  case 43:
#line 334 "rl78-parse.y"
    { B2 (0x61, 0xcb); }
    break;

  case 44:
#line 337 "rl78-parse.y"
    { B1 (0xef); PC1 ((yyvsp[(3) - (3)].exp)); }
    break;

  case 45:
#line 340 "rl78-parse.y"
    { B1 (0xee); PC2 ((yyvsp[(4) - (4)].exp)); rl78_linkrelax_branch (); }
    break;

  case 46:
#line 343 "rl78-parse.y"
    { B1 (0xed); O2 ((yyvsp[(3) - (3)].exp)); rl78_linkrelax_branch (); }
    break;

  case 47:
#line 346 "rl78-parse.y"
    { B1 (0xec); O3 ((yyvsp[(4) - (4)].exp)); rl78_linkrelax_branch (); }
    break;

  case 48:
#line 351 "rl78-parse.y"
    { B2 (0x61, 0xcc); }
    break;

  case 49:
#line 354 "rl78-parse.y"
    { B1 (0xff); }
    break;

  case 50:
#line 359 "rl78-parse.y"
    { B2 (0x61, 0xca); F ((yyvsp[(2) - (2)].regno), 10, 2); }
    break;

  case 51:
#line 362 "rl78-parse.y"
    { B1 (0xfe); PC2 ((yyvsp[(4) - (4)].exp)); }
    break;

  case 52:
#line 365 "rl78-parse.y"
    { B1 (0xfd); O2 ((yyvsp[(3) - (3)].exp)); }
    break;

  case 53:
#line 368 "rl78-parse.y"
    { B1 (0xfc); O3 ((yyvsp[(4) - (4)].exp)); rl78_linkrelax_branch (); }
    break;

  case 54:
#line 371 "rl78-parse.y"
    { if ((yyvsp[(3) - (4)].exp).X_op != O_constant)
	      rl78_error ("CALLT requires a numeric address");
	    else
	      {
	        int i = (yyvsp[(3) - (4)].exp).X_add_number;
		if (i < 0x80 || i > 0xbe)
		  rl78_error ("CALLT address not 0x80..0xbe");
		else if (i & 1)
		  rl78_error ("CALLT address not even");
		else
		  {
		    B2 (0x61, 0x84);
	    	    F ((i >> 1) & 7, 9, 3);
	    	    F ((i >> 4) & 7, 14, 2);
		  }
	      }
	  }
    break;

  case 55:
#line 392 "rl78-parse.y"
    { B2 (0x71, (yyvsp[(1) - (2)].regno) ? 0x88 : 0x80); }
    break;

  case 56:
#line 395 "rl78-parse.y"
    { B3 (0x71, 0x0a|(yyvsp[(1) - (4)].regno), (yyvsp[(2) - (4)].regno)); FE ((yyvsp[(4) - (4)].exp), 9, 3); }
    break;

  case 57:
#line 398 "rl78-parse.y"
    { if (expr_is_sfr ((yyvsp[(2) - (4)].exp)))
	      { B2 (0x71, 0x0a|(yyvsp[(1) - (4)].regno)); FE ((yyvsp[(4) - (4)].exp), 9, 3); O1 ((yyvsp[(2) - (4)].exp)); }
	    else if (expr_is_saddr ((yyvsp[(2) - (4)].exp)))
	      { B2 (0x71, 0x02|(yyvsp[(1) - (4)].regno)); FE ((yyvsp[(4) - (4)].exp), 9, 3); O1 ((yyvsp[(2) - (4)].exp)); }
	    else
	      NOT_SFR_OR_SADDR;
	  }
    break;

  case 58:
#line 407 "rl78-parse.y"
    { B2 (0x71, 0x8a|(yyvsp[(1) - (4)].regno));  FE ((yyvsp[(4) - (4)].exp), 9, 3); }
    break;

  case 59:
#line 410 "rl78-parse.y"
    { B2 (0x71, 0x00+(yyvsp[(1) - (6)].regno)*0x08); FE ((yyvsp[(6) - (6)].exp), 9, 3); O2 ((yyvsp[(4) - (6)].exp)); rl78_linkrelax_addr16 (); }
    break;

  case 60:
#line 413 "rl78-parse.y"
    { B2 (0x71, 0x82|(yyvsp[(1) - (7)].regno)); FE ((yyvsp[(7) - (7)].exp), 9, 3); }
    break;

  case 61:
#line 418 "rl78-parse.y"
    { B1 (0xe1|(yyvsp[(1) - (2)].regno)); }
    break;

  case 62:
#line 420 "rl78-parse.y"
    { B1 (0xe0|(yyvsp[(1) - (2)].regno)); }
    break;

  case 63:
#line 422 "rl78-parse.y"
    { B1 (0xe3|(yyvsp[(1) - (2)].regno)); }
    break;

  case 64:
#line 424 "rl78-parse.y"
    { B1 (0xe2|(yyvsp[(1) - (2)].regno)); }
    break;

  case 65:
#line 426 "rl78-parse.y"
    {SA((yyvsp[(2) - (2)].exp))}
    break;

  case 66:
#line 427 "rl78-parse.y"
    { B1 (0xe4|(yyvsp[(1) - (3)].regno)); O1 ((yyvsp[(2) - (3)].exp)); }
    break;

  case 67:
#line 430 "rl78-parse.y"
    { B1 (0xe5|(yyvsp[(1) - (4)].regno)); O2 ((yyvsp[(4) - (4)].exp)); rl78_linkrelax_addr16 (); }
    break;

  case 68:
#line 435 "rl78-parse.y"
    { B1 (0xe6|(yyvsp[(1) - (2)].regno)); }
    break;

  case 69:
#line 437 "rl78-parse.y"
    { B1 (0xe7|(yyvsp[(1) - (2)].regno)); }
    break;

  case 70:
#line 442 "rl78-parse.y"
    { B1 (0xd1); }
    break;

  case 71:
#line 445 "rl78-parse.y"
    { B1 (0xd0); }
    break;

  case 72:
#line 448 "rl78-parse.y"
    { B1 (0xd3); }
    break;

  case 73:
#line 451 "rl78-parse.y"
    { B1 (0xd2); }
    break;

  case 74:
#line 453 "rl78-parse.y"
    {SA((yyvsp[(2) - (2)].exp))}
    break;

  case 75:
#line 454 "rl78-parse.y"
    { B1 (0xd4); O1 ((yyvsp[(2) - (3)].exp)); }
    break;

  case 76:
#line 457 "rl78-parse.y"
    { B1 (0xd5); O2 ((yyvsp[(4) - (4)].exp)); rl78_linkrelax_addr16 (); }
    break;

  case 77:
#line 462 "rl78-parse.y"
    { B2 (0x61, 0xde); O1 ((yyvsp[(8) - (9)].exp)); }
    break;

  case 78:
#line 467 "rl78-parse.y"
    { B1 (0x80|(yyvsp[(1) - (2)].regno)); F ((yyvsp[(2) - (2)].regno), 5, 3); }
    break;

  case 79:
#line 469 "rl78-parse.y"
    {SA((yyvsp[(2) - (2)].exp))}
    break;

  case 80:
#line 470 "rl78-parse.y"
    { B1 (0xa4|(yyvsp[(1) - (3)].regno)); O1 ((yyvsp[(2) - (3)].exp)); }
    break;

  case 81:
#line 472 "rl78-parse.y"
    { B1 (0xa0|(yyvsp[(1) - (3)].regno)); O2 ((yyvsp[(3) - (3)].exp)); rl78_linkrelax_addr16 (); }
    break;

  case 82:
#line 474 "rl78-parse.y"
    { B2 (0x11, 0xa0|(yyvsp[(1) - (5)].regno)); O2 ((yyvsp[(5) - (5)].exp)); }
    break;

  case 83:
#line 476 "rl78-parse.y"
    { B2 (0x61, 0x59+(yyvsp[(1) - (6)].regno)); O1 ((yyvsp[(5) - (6)].exp)); }
    break;

  case 84:
#line 478 "rl78-parse.y"
    { B3 (0x11, 0x61, 0x59+(yyvsp[(1) - (8)].regno)); O1 ((yyvsp[(7) - (8)].exp)); }
    break;

  case 85:
#line 483 "rl78-parse.y"
    { B1 (0xa1|(yyvsp[(1) - (2)].regno)); F ((yyvsp[(2) - (2)].regno), 5, 2); }
    break;

  case 86:
#line 485 "rl78-parse.y"
    {SA((yyvsp[(2) - (2)].exp))}
    break;

  case 87:
#line 486 "rl78-parse.y"
    { B1 (0xa6|(yyvsp[(1) - (3)].regno)); O1 ((yyvsp[(2) - (3)].exp)); }
    break;

  case 88:
#line 489 "rl78-parse.y"
    { B1 (0xa2|(yyvsp[(1) - (4)].regno)); O2 ((yyvsp[(4) - (4)].exp)); rl78_linkrelax_addr16 (); }
    break;

  case 89:
#line 492 "rl78-parse.y"
    { B2 (0x61, 0x79+(yyvsp[(1) - (7)].regno)); O1 ((yyvsp[(6) - (7)].exp)); }
    break;

  case 90:
#line 497 "rl78-parse.y"
    { B3 (0x71, 0x7b, 0xfa); }
    break;

  case 91:
#line 500 "rl78-parse.y"
    { B3 (0x71, 0x7a, 0xfa); }
    break;

  case 92:
#line 505 "rl78-parse.y"
    { B3 (0xce, 0xfb, 0x01); }
    break;

  case 93:
#line 508 "rl78-parse.y"
    { B3 (0xce, 0xfb, 0x02); }
    break;

  case 94:
#line 511 "rl78-parse.y"
    { B1 (0xd6); }
    break;

  case 95:
#line 514 "rl78-parse.y"
    { B3 (0xce, 0xfb, 0x03); }
    break;

  case 96:
#line 517 "rl78-parse.y"
    { B3 (0xce, 0xfb, 0x04); }
    break;

  case 97:
#line 520 "rl78-parse.y"
    { B3 (0xce, 0xfb, 0x05); }
    break;

  case 98:
#line 523 "rl78-parse.y"
    { B3 (0xce, 0xfb, 0x06); }
    break;

  case 99:
#line 528 "rl78-parse.y"
    { B2 (0x61, 0xed); }
    break;

  case 100:
#line 536 "rl78-parse.y"
    { B1 (0x51); O1 ((yyvsp[(5) - (5)].exp)); }
    break;

  case 101:
#line 538 "rl78-parse.y"
    { B1 (0x50); F((yyvsp[(2) - (5)].regno), 5, 3); O1 ((yyvsp[(5) - (5)].exp)); }
    break;

  case 102:
#line 541 "rl78-parse.y"
    { if ((yyvsp[(2) - (5)].regno) != 0xfd)
	      { B2 (0xce, (yyvsp[(2) - (5)].regno)); O1 ((yyvsp[(5) - (5)].exp)); }
	    else
	      { B1 (0x41); O1 ((yyvsp[(5) - (5)].exp)); }
	  }
    break;

  case 103:
#line 547 "rl78-parse.y"
    {NOT_ES}
    break;

  case 104:
#line 548 "rl78-parse.y"
    { if (expr_is_sfr ((yyvsp[(3) - (7)].exp)))
	      { B1 (0xce); O1 ((yyvsp[(3) - (7)].exp)); O1 ((yyvsp[(6) - (7)].exp)); }
	    else if (expr_is_saddr ((yyvsp[(3) - (7)].exp)))
	      { B1 (0xcd); O1 ((yyvsp[(3) - (7)].exp)); O1 ((yyvsp[(6) - (7)].exp)); }
	    else
	      NOT_SFR_OR_SADDR;
	  }
    break;

  case 105:
#line 557 "rl78-parse.y"
    { B1 (0xcf); O2 ((yyvsp[(3) - (6)].exp)); O1 ((yyvsp[(6) - (6)].exp)); rl78_linkrelax_addr16 (); }
    break;

  case 106:
#line 560 "rl78-parse.y"
    { B2 (0x11, 0xcf); O2 ((yyvsp[(5) - (8)].exp)); O1 ((yyvsp[(8) - (8)].exp)); }
    break;

  case 107:
#line 563 "rl78-parse.y"
    { B1 (0x70); F ((yyvsp[(2) - (4)].regno), 5, 3); }
    break;

  case 108:
#line 566 "rl78-parse.y"
    { B1 (0x60); F ((yyvsp[(4) - (4)].regno), 5, 3); }
    break;

  case 109:
#line 568 "rl78-parse.y"
    {NOT_ES}
    break;

  case 110:
#line 569 "rl78-parse.y"
    { if (expr_is_sfr ((yyvsp[(3) - (6)].exp)))
	      { B1 (0x9e); O1 ((yyvsp[(3) - (6)].exp)); }
	    else if (expr_is_saddr ((yyvsp[(3) - (6)].exp)))
	      { B1 (0x9d); O1 ((yyvsp[(3) - (6)].exp)); }
	    else
	      NOT_SFR_OR_SADDR;
	  }
    break;

  case 111:
#line 578 "rl78-parse.y"
    { B1 (0x8f); O2 ((yyvsp[(6) - (6)].exp)); rl78_linkrelax_addr16 (); }
    break;

  case 112:
#line 581 "rl78-parse.y"
    { B1 (0x9f); O2 ((yyvsp[(3) - (5)].exp)); rl78_linkrelax_addr16 (); }
    break;

  case 113:
#line 584 "rl78-parse.y"
    { B2 (0x11, 0x9f); O2 ((yyvsp[(5) - (7)].exp)); }
    break;

  case 114:
#line 587 "rl78-parse.y"
    { B1 (0xc9|reg_xbc((yyvsp[(2) - (6)].regno))); O2 ((yyvsp[(6) - (6)].exp)); rl78_linkrelax_addr16 (); }
    break;

  case 115:
#line 589 "rl78-parse.y"
    {NOT_ES}
    break;

  case 116:
#line 590 "rl78-parse.y"
    { if (expr_is_saddr ((yyvsp[(5) - (6)].exp)))
	      { B1 (0x8d); O1 ((yyvsp[(5) - (6)].exp)); }
	    else if (expr_is_sfr ((yyvsp[(5) - (6)].exp)))
	      { B1 (0x8e); O1 ((yyvsp[(5) - (6)].exp)); }
	    else
	      NOT_SFR_OR_SADDR;
	  }
    break;

  case 117:
#line 598 "rl78-parse.y"
    {SA((yyvsp[(5) - (5)].exp))}
    break;

  case 118:
#line 598 "rl78-parse.y"
    {NOT_ES}
    break;

  case 119:
#line 599 "rl78-parse.y"
    { B1 (0xc8|reg_xbc((yyvsp[(2) - (7)].regno))); O1 ((yyvsp[(5) - (7)].exp)); }
    break;

  case 120:
#line 602 "rl78-parse.y"
    { B2 (0x8e, (yyvsp[(4) - (4)].regno)); }
    break;

  case 121:
#line 605 "rl78-parse.y"
    { if ((yyvsp[(4) - (4)].regno) != 1)
	      rl78_error ("Only A allowed here");
	    else
	      { B2 (0x9e, (yyvsp[(2) - (4)].regno)); }
	  }
    break;

  case 122:
#line 611 "rl78-parse.y"
    {SA((yyvsp[(5) - (5)].exp))}
    break;

  case 123:
#line 611 "rl78-parse.y"
    {NOT_ES}
    break;

  case 124:
#line 612 "rl78-parse.y"
    { if ((yyvsp[(2) - (7)].regno) != 0xfd)
	      rl78_error ("Only ES allowed here");
	    else
	      { B2 (0x61, 0xb8); O1 ((yyvsp[(5) - (7)].exp)); }
	  }
    break;

  case 125:
#line 619 "rl78-parse.y"
    { B1 (0x89); }
    break;

  case 126:
#line 622 "rl78-parse.y"
    { B1 (0x99); }
    break;

  case 127:
#line 625 "rl78-parse.y"
    { B1 (0xca); O1 ((yyvsp[(6) - (10)].exp)); O1 ((yyvsp[(10) - (10)].exp)); }
    break;

  case 128:
#line 628 "rl78-parse.y"
    { B1 (0x8a); O1 ((yyvsp[(8) - (9)].exp)); }
    break;

  case 129:
#line 631 "rl78-parse.y"
    { B1 (0x9a); O1 ((yyvsp[(6) - (9)].exp)); }
    break;

  case 130:
#line 634 "rl78-parse.y"
    { B1 (0x8b); }
    break;

  case 131:
#line 637 "rl78-parse.y"
    { B1 (0x9b); }
    break;

  case 132:
#line 640 "rl78-parse.y"
    { B1 (0xcc); O1 ((yyvsp[(6) - (10)].exp)); O1 ((yyvsp[(10) - (10)].exp)); }
    break;

  case 133:
#line 643 "rl78-parse.y"
    { B1 (0x8c); O1 ((yyvsp[(8) - (9)].exp)); }
    break;

  case 134:
#line 646 "rl78-parse.y"
    { B1 (0x9c); O1 ((yyvsp[(6) - (9)].exp)); }
    break;

  case 135:
#line 649 "rl78-parse.y"
    { B2 (0x61, 0xc9); }
    break;

  case 136:
#line 652 "rl78-parse.y"
    { B2 (0x61, 0xd9); }
    break;

  case 137:
#line 655 "rl78-parse.y"
    { B2 (0x61, 0xe9); }
    break;

  case 138:
#line 658 "rl78-parse.y"
    { B2 (0x61, 0xf9); }
    break;

  case 139:
#line 661 "rl78-parse.y"
    { B1 (0x19); O2 ((yyvsp[(3) - (9)].exp)); O1 ((yyvsp[(9) - (9)].exp)); }
    break;

  case 140:
#line 664 "rl78-parse.y"
    { B1 (0x09); O2 ((yyvsp[(5) - (8)].exp)); }
    break;

  case 141:
#line 667 "rl78-parse.y"
    { B1 (0x18); O2 ((yyvsp[(3) - (8)].exp)); }
    break;

  case 142:
#line 670 "rl78-parse.y"
    { B1 (0x38); O2 ((yyvsp[(3) - (9)].exp)); O1 ((yyvsp[(9) - (9)].exp)); }
    break;

  case 143:
#line 673 "rl78-parse.y"
    { B1 (0x29); O2 ((yyvsp[(5) - (8)].exp)); }
    break;

  case 144:
#line 676 "rl78-parse.y"
    { B1 (0x28); O2 ((yyvsp[(3) - (8)].exp)); }
    break;

  case 145:
#line 679 "rl78-parse.y"
    { B1 (0x39); O2 ((yyvsp[(3) - (9)].exp)); O1 ((yyvsp[(9) - (9)].exp)); }
    break;

  case 146:
#line 682 "rl78-parse.y"
    { B3 (0x39, 0, 0); O1 ((yyvsp[(8) - (8)].exp)); }
    break;

  case 147:
#line 685 "rl78-parse.y"
    { B1 (0x49); O2 ((yyvsp[(5) - (8)].exp)); }
    break;

  case 148:
#line 688 "rl78-parse.y"
    { B3 (0x49, 0, 0); }
    break;

  case 149:
#line 691 "rl78-parse.y"
    { B1 (0x48); O2 ((yyvsp[(3) - (8)].exp)); }
    break;

  case 150:
#line 694 "rl78-parse.y"
    { B3 (0x48, 0, 0); }
    break;

  case 151:
#line 696 "rl78-parse.y"
    {NOT_ES}
    break;

  case 152:
#line 697 "rl78-parse.y"
    { B1 (0xc8); O1 ((yyvsp[(6) - (11)].exp)); O1 ((yyvsp[(10) - (11)].exp)); }
    break;

  case 153:
#line 699 "rl78-parse.y"
    {NOT_ES}
    break;

  case 154:
#line 700 "rl78-parse.y"
    { B2 (0xc8, 0); O1 ((yyvsp[(8) - (9)].exp)); }
    break;

  case 155:
#line 702 "rl78-parse.y"
    {NOT_ES}
    break;

  case 156:
#line 703 "rl78-parse.y"
    { B1 (0x88); O1 ((yyvsp[(8) - (10)].exp)); }
    break;

  case 157:
#line 705 "rl78-parse.y"
    {NOT_ES}
    break;

  case 158:
#line 706 "rl78-parse.y"
    { B2 (0x88, 0); }
    break;

  case 159:
#line 708 "rl78-parse.y"
    {NOT_ES}
    break;

  case 160:
#line 709 "rl78-parse.y"
    { B1 (0x98); O1 ((yyvsp[(6) - (10)].exp)); }
    break;

  case 161:
#line 711 "rl78-parse.y"
    {NOT_ES}
    break;

  case 162:
#line 712 "rl78-parse.y"
    { B2 (0x98, 0); }
    break;

  case 163:
#line 717 "rl78-parse.y"
    { if (expr_is_saddr ((yyvsp[(4) - (6)].exp)))
	      { B2 (0x71, 0x04); FE ((yyvsp[(6) - (6)].exp), 9, 3); O1 ((yyvsp[(4) - (6)].exp)); }
	    else if (expr_is_sfr ((yyvsp[(4) - (6)].exp)))
	      { B2 (0x71, 0x0c); FE ((yyvsp[(6) - (6)].exp), 9, 3); O1 ((yyvsp[(4) - (6)].exp)); }
	    else
	      NOT_SFR_OR_SADDR;
	  }
    break;

  case 164:
#line 726 "rl78-parse.y"
    { B2 (0x71, 0x8c); FE ((yyvsp[(6) - (6)].exp), 9, 3); }
    break;

  case 165:
#line 729 "rl78-parse.y"
    { B3 (0x71, 0x0c, (yyvsp[(4) - (6)].regno)); FE ((yyvsp[(6) - (6)].exp), 9, 3); }
    break;

  case 166:
#line 732 "rl78-parse.y"
    { B2 (0x71, 0x84); FE ((yyvsp[(9) - (9)].exp), 9, 3); }
    break;

  case 167:
#line 735 "rl78-parse.y"
    { if (expr_is_saddr ((yyvsp[(2) - (6)].exp)))
	      { B2 (0x71, 0x01); FE ((yyvsp[(4) - (6)].exp), 9, 3); O1 ((yyvsp[(2) - (6)].exp)); }
	    else if (expr_is_sfr ((yyvsp[(2) - (6)].exp)))
	      { B2 (0x71, 0x09); FE ((yyvsp[(4) - (6)].exp), 9, 3); O1 ((yyvsp[(2) - (6)].exp)); }
	    else
	      NOT_SFR_OR_SADDR;
	  }
    break;

  case 168:
#line 744 "rl78-parse.y"
    { B2 (0x71, 0x89); FE ((yyvsp[(4) - (6)].exp), 9, 3); }
    break;

  case 169:
#line 747 "rl78-parse.y"
    { B3 (0x71, 0x09, (yyvsp[(2) - (6)].regno)); FE ((yyvsp[(4) - (6)].exp), 9, 3); }
    break;

  case 170:
#line 750 "rl78-parse.y"
    { B2 (0x71, 0x81); FE ((yyvsp[(7) - (9)].exp), 9, 3); }
    break;

  case 171:
#line 755 "rl78-parse.y"
    { B2 (0x61, 0xce); O1 ((yyvsp[(6) - (9)].exp)); }
    break;

  case 172:
#line 760 "rl78-parse.y"
    { B1 (0x30); O2 ((yyvsp[(5) - (5)].exp)); }
    break;

  case 173:
#line 763 "rl78-parse.y"
    { B1 (0x30); F ((yyvsp[(2) - (5)].regno), 5, 2); O2 ((yyvsp[(5) - (5)].exp)); }
    break;

  case 174:
#line 765 "rl78-parse.y"
    {NOT_ES}
    break;

  case 175:
#line 766 "rl78-parse.y"
    { if (expr_is_saddr ((yyvsp[(3) - (7)].exp)))
	      { B1 (0xc9); O1 ((yyvsp[(3) - (7)].exp)); O2 ((yyvsp[(6) - (7)].exp)); }
	    else if (expr_is_sfr ((yyvsp[(3) - (7)].exp)))
	      { B1 (0xcb); O1 ((yyvsp[(3) - (7)].exp)); O2 ((yyvsp[(6) - (7)].exp)); }
	    else
	      NOT_SFR_OR_SADDR;
	  }
    break;

  case 176:
#line 774 "rl78-parse.y"
    {NOT_ES}
    break;

  case 177:
#line 775 "rl78-parse.y"
    { if (expr_is_saddr ((yyvsp[(5) - (6)].exp)))
	      { B1 (0xad); O1 ((yyvsp[(5) - (6)].exp)); WA((yyvsp[(5) - (6)].exp)); }
	    else if (expr_is_sfr ((yyvsp[(5) - (6)].exp)))
	      { B1 (0xae); O1 ((yyvsp[(5) - (6)].exp)); WA((yyvsp[(5) - (6)].exp)); }
	    else
	      NOT_SFR_OR_SADDR;
	  }
    break;

  case 178:
#line 783 "rl78-parse.y"
    {NOT_ES}
    break;

  case 179:
#line 784 "rl78-parse.y"
    { if (expr_is_saddr ((yyvsp[(3) - (6)].exp)))
	      { B1 (0xbd); O1 ((yyvsp[(3) - (6)].exp)); WA((yyvsp[(3) - (6)].exp)); }
	    else if (expr_is_sfr ((yyvsp[(3) - (6)].exp)))
	      { B1 (0xbe); O1 ((yyvsp[(3) - (6)].exp)); WA((yyvsp[(3) - (6)].exp)); }
	    else
	      NOT_SFR_OR_SADDR;
	  }
    break;

  case 180:
#line 793 "rl78-parse.y"
    { B1 (0x11); F ((yyvsp[(4) - (4)].regno), 5, 2); }
    break;

  case 181:
#line 796 "rl78-parse.y"
    { B1 (0x10); F ((yyvsp[(2) - (4)].regno), 5, 2); }
    break;

  case 182:
#line 799 "rl78-parse.y"
    { B1 (0xaf); O2 ((yyvsp[(6) - (6)].exp)); WA((yyvsp[(6) - (6)].exp)); rl78_linkrelax_addr16 (); }
    break;

  case 183:
#line 802 "rl78-parse.y"
    { B1 (0xbf); O2 ((yyvsp[(4) - (6)].exp)); WA((yyvsp[(4) - (6)].exp)); rl78_linkrelax_addr16 (); }
    break;

  case 184:
#line 805 "rl78-parse.y"
    { B1 (0xa9); }
    break;

  case 185:
#line 808 "rl78-parse.y"
    { B1 (0xb9); }
    break;

  case 186:
#line 811 "rl78-parse.y"
    { B1 (0xaa); O1 ((yyvsp[(8) - (9)].exp)); }
    break;

  case 187:
#line 814 "rl78-parse.y"
    { B1 (0xba); O1 ((yyvsp[(6) - (9)].exp)); }
    break;

  case 188:
#line 817 "rl78-parse.y"
    { B1 (0xab); }
    break;

  case 189:
#line 820 "rl78-parse.y"
    { B1 (0xbb); }
    break;

  case 190:
#line 823 "rl78-parse.y"
    { B1 (0xac); O1 ((yyvsp[(8) - (9)].exp)); }
    break;

  case 191:
#line 826 "rl78-parse.y"
    { B1 (0xbc); O1 ((yyvsp[(6) - (9)].exp)); }
    break;

  case 192:
#line 829 "rl78-parse.y"
    { B1 (0x59); O2 ((yyvsp[(5) - (8)].exp)); }
    break;

  case 193:
#line 832 "rl78-parse.y"
    { B1 (0x58); O2 ((yyvsp[(3) - (8)].exp)); }
    break;

  case 194:
#line 835 "rl78-parse.y"
    { B1 (0x69); O2 ((yyvsp[(5) - (8)].exp)); }
    break;

  case 195:
#line 838 "rl78-parse.y"
    { B1 (0x68); O2 ((yyvsp[(3) - (8)].exp)); }
    break;

  case 196:
#line 841 "rl78-parse.y"
    { B1 (0x79); O2 ((yyvsp[(5) - (8)].exp)); }
    break;

  case 197:
#line 844 "rl78-parse.y"
    { B3 (0x79, 0, 0); }
    break;

  case 198:
#line 847 "rl78-parse.y"
    { B1 (0x78); O2 ((yyvsp[(3) - (8)].exp)); }
    break;

  case 199:
#line 850 "rl78-parse.y"
    { B3 (0x78, 0, 0); }
    break;

  case 200:
#line 852 "rl78-parse.y"
    {NOT_ES}
    break;

  case 201:
#line 853 "rl78-parse.y"
    { B1 (0xa8); O1 ((yyvsp[(8) - (10)].exp));  WA((yyvsp[(8) - (10)].exp));}
    break;

  case 202:
#line 855 "rl78-parse.y"
    {NOT_ES}
    break;

  case 203:
#line 856 "rl78-parse.y"
    { B2 (0xa8, 0); }
    break;

  case 204:
#line 858 "rl78-parse.y"
    {NOT_ES}
    break;

  case 205:
#line 859 "rl78-parse.y"
    { B1 (0xb8); O1 ((yyvsp[(6) - (10)].exp)); WA((yyvsp[(6) - (10)].exp)); }
    break;

  case 206:
#line 861 "rl78-parse.y"
    {NOT_ES}
    break;

  case 207:
#line 862 "rl78-parse.y"
    { B2 (0xb8, 0); }
    break;

  case 208:
#line 864 "rl78-parse.y"
    {SA((yyvsp[(4) - (4)].exp))}
    break;

  case 209:
#line 865 "rl78-parse.y"
    { B1 (0xca); F ((yyvsp[(2) - (5)].regno), 2, 2); O1 ((yyvsp[(4) - (5)].exp)); WA((yyvsp[(4) - (5)].exp)); }
    break;

  case 210:
#line 868 "rl78-parse.y"
    { B1 (0xcb); F ((yyvsp[(2) - (6)].regno), 2, 2); O2 ((yyvsp[(6) - (6)].exp)); WA((yyvsp[(6) - (6)].exp)); rl78_linkrelax_addr16 (); }
    break;

  case 211:
#line 871 "rl78-parse.y"
    { B2 (0xcb, 0xf8); O2 ((yyvsp[(5) - (5)].exp)); }
    break;

  case 212:
#line 874 "rl78-parse.y"
    { B2 (0xbe, 0xf8); }
    break;

  case 213:
#line 877 "rl78-parse.y"
    { B2 (0xae, 0xf8); }
    break;

  case 214:
#line 880 "rl78-parse.y"
    { B3 (0xcb, 0xf8, 0xff); F ((yyvsp[(2) - (4)].regno), 2, 2); }
    break;

  case 215:
#line 885 "rl78-parse.y"
    { B1 (0x00); }
    break;

  case 216:
#line 890 "rl78-parse.y"
    { B2 (0x71, 0xc0); }
    break;

  case 217:
#line 895 "rl78-parse.y"
    { B1 (0xc0); F ((yyvsp[(2) - (2)].regno), 5, 2); }
    break;

  case 218:
#line 898 "rl78-parse.y"
    { B2 (0x61, 0xcd); }
    break;

  case 219:
#line 901 "rl78-parse.y"
    { B1 (0xc1); F ((yyvsp[(2) - (2)].regno), 5, 2); }
    break;

  case 220:
#line 904 "rl78-parse.y"
    { B2 (0x61, 0xdd); }
    break;

  case 221:
#line 909 "rl78-parse.y"
    { B1 (0xd7); }
    break;

  case 222:
#line 912 "rl78-parse.y"
    { B2 (0x61, 0xfc); }
    break;

  case 223:
#line 915 "rl78-parse.y"
    { B2 (0x61, 0xec); }
    break;

  case 224:
#line 920 "rl78-parse.y"
    { if (check_expr_is_const ((yyvsp[(4) - (4)].exp), 1, 1))
	      { B2 (0x61, 0xeb); }
	  }
    break;

  case 225:
#line 925 "rl78-parse.y"
    { if (check_expr_is_const ((yyvsp[(4) - (4)].exp), 1, 1))
	      { B2 (0x61, 0xdc); }
	  }
    break;

  case 226:
#line 930 "rl78-parse.y"
    { if (check_expr_is_const ((yyvsp[(4) - (4)].exp), 1, 1))
	      { B2 (0x61, 0xee); }
	  }
    break;

  case 227:
#line 935 "rl78-parse.y"
    { if (check_expr_is_const ((yyvsp[(4) - (4)].exp), 1, 1))
	      { B2 (0x61, 0xfe); }
	  }
    break;

  case 228:
#line 940 "rl78-parse.y"
    { if (check_expr_is_const ((yyvsp[(4) - (4)].exp), 1, 1))
	      { B2 (0x61, 0xdb); }
	  }
    break;

  case 229:
#line 945 "rl78-parse.y"
    { if (check_expr_is_const ((yyvsp[(4) - (4)].exp), 1, 1))
	      { B2 (0x61, 0xfb);}
	  }
    break;

  case 230:
#line 952 "rl78-parse.y"
    { if (check_expr_is_const ((yyvsp[(4) - (4)].exp), 1, 7))
	      { B2 (0x31, 0x0b); FE ((yyvsp[(4) - (4)].exp), 9, 3); }
	  }
    break;

  case 231:
#line 957 "rl78-parse.y"
    { if (check_expr_is_const ((yyvsp[(4) - (4)].exp), 1, 15))
	      { B2 (0x31, 0x0f); FE ((yyvsp[(4) - (4)].exp), 8, 4); }
	  }
    break;

  case 232:
#line 964 "rl78-parse.y"
    { B2 (0x61, 0xcf); }
    break;

  case 233:
#line 967 "rl78-parse.y"
    { B2 (0x61, 0xdf); }
    break;

  case 234:
#line 970 "rl78-parse.y"
    { B2 (0x61, 0xef); }
    break;

  case 235:
#line 973 "rl78-parse.y"
    { B2 (0x61, 0xff); }
    break;

  case 236:
#line 978 "rl78-parse.y"
    { if (check_expr_is_const ((yyvsp[(4) - (4)].exp), 1, 7))
	      { B2 (0x31, 0x09); FE ((yyvsp[(4) - (4)].exp), 9, 3); }
	  }
    break;

  case 237:
#line 983 "rl78-parse.y"
    { if (check_expr_is_const ((yyvsp[(4) - (4)].exp), 1, 7))
	      { B2 (0x31, 0x08); FE ((yyvsp[(4) - (4)].exp), 9, 3); }
	  }
    break;

  case 238:
#line 988 "rl78-parse.y"
    { if (check_expr_is_const ((yyvsp[(4) - (4)].exp), 1, 7))
	      { B2 (0x31, 0x07); FE ((yyvsp[(4) - (4)].exp), 9, 3); }
	  }
    break;

  case 239:
#line 993 "rl78-parse.y"
    { if (check_expr_is_const ((yyvsp[(4) - (4)].exp), 1, 15))
	      { B2 (0x31, 0x0d); FE ((yyvsp[(4) - (4)].exp), 8, 4); }
	  }
    break;

  case 240:
#line 998 "rl78-parse.y"
    { if (check_expr_is_const ((yyvsp[(4) - (4)].exp), 1, 15))
	      { B2 (0x31, 0x0c); FE ((yyvsp[(4) - (4)].exp), 8, 4); }
	  }
    break;

  case 241:
#line 1005 "rl78-parse.y"
    { if (check_expr_is_const ((yyvsp[(4) - (4)].exp), 1, 7))
	      { B2 (0x31, 0x0a); FE ((yyvsp[(4) - (4)].exp), 9, 3); }
	  }
    break;

  case 242:
#line 1010 "rl78-parse.y"
    { if (check_expr_is_const ((yyvsp[(4) - (4)].exp), 1, 15))
	      { B2 (0x31, 0x0e); FE ((yyvsp[(4) - (4)].exp), 8, 4); }
	  }
    break;

  case 243:
#line 1017 "rl78-parse.y"
    { B2 (0x61, 0xc8); rl78_linkrelax_branch (); }
    break;

  case 244:
#line 1020 "rl78-parse.y"
    { B2 (0x61, 0xe3); rl78_linkrelax_branch (); }
    break;

  case 245:
#line 1023 "rl78-parse.y"
    { B2 (0x61, 0xd8); rl78_linkrelax_branch (); }
    break;

  case 246:
#line 1026 "rl78-parse.y"
    { B2 (0x61, 0xf3); rl78_linkrelax_branch (); }
    break;

  case 247:
#line 1029 "rl78-parse.y"
    { B2 (0x61, 0xf8); rl78_linkrelax_branch (); }
    break;

  case 248:
#line 1032 "rl78-parse.y"
    { B2 (0x61, 0xe8); rl78_linkrelax_branch (); }
    break;

  case 249:
#line 1037 "rl78-parse.y"
    { B2 (0x61, 0xfd); }
    break;

  case 250:
#line 1042 "rl78-parse.y"
    { if ((yyvsp[(4) - (4)].regno) == 0) /* X */
	      { B1 (0x08); }
	    else
	      { B2 (0x61, 0x88); F ((yyvsp[(4) - (4)].regno), 13, 3); }
	  }
    break;

  case 251:
#line 1049 "rl78-parse.y"
    { B2 (0x61, 0xaa); O2 ((yyvsp[(6) - (6)].exp)); rl78_linkrelax_addr16 (); }
    break;

  case 252:
#line 1052 "rl78-parse.y"
    { B2 (0x61, 0xae); }
    break;

  case 253:
#line 1055 "rl78-parse.y"
    { B2 (0x61, 0xaf); O1 ((yyvsp[(8) - (9)].exp)); }
    break;

  case 254:
#line 1058 "rl78-parse.y"
    { B2 (0x61, 0xac); }
    break;

  case 255:
#line 1061 "rl78-parse.y"
    { B2 (0x61, 0xad); O1 ((yyvsp[(8) - (9)].exp)); }
    break;

  case 256:
#line 1064 "rl78-parse.y"
    { B2 (0x61, 0xb9); }
    break;

  case 257:
#line 1067 "rl78-parse.y"
    { B2 (0x61, 0xa9); }
    break;

  case 258:
#line 1070 "rl78-parse.y"
    { if (expr_is_sfr ((yyvsp[(4) - (4)].exp)))
	      { B2 (0x61, 0xab); O1 ((yyvsp[(4) - (4)].exp)); }
	    else if (expr_is_saddr ((yyvsp[(4) - (4)].exp)))
	      { B2 (0x61, 0xa8); O1 ((yyvsp[(4) - (4)].exp)); }
	    else
	      NOT_SFR_OR_SADDR;
	  }
    break;

  case 259:
#line 1081 "rl78-parse.y"
    { B1 (0x31); F ((yyvsp[(4) - (4)].regno), 5, 2); }
    break;

  case 261:
#line 1091 "rl78-parse.y"
    { rl78_prefix (0x11); }
    break;

  case 262:
#line 1094 "rl78-parse.y"
    { (yyval.regno) = 0; }
    break;

  case 263:
#line 1095 "rl78-parse.y"
    { (yyval.regno) = 1; }
    break;

  case 264:
#line 1096 "rl78-parse.y"
    { (yyval.regno) = 2; }
    break;

  case 265:
#line 1097 "rl78-parse.y"
    { (yyval.regno) = 3; }
    break;

  case 266:
#line 1098 "rl78-parse.y"
    { (yyval.regno) = 4; }
    break;

  case 267:
#line 1099 "rl78-parse.y"
    { (yyval.regno) = 5; }
    break;

  case 268:
#line 1100 "rl78-parse.y"
    { (yyval.regno) = 6; }
    break;

  case 269:
#line 1101 "rl78-parse.y"
    { (yyval.regno) = 7; }
    break;

  case 270:
#line 1104 "rl78-parse.y"
    { (yyval.regno) = 0; }
    break;

  case 271:
#line 1105 "rl78-parse.y"
    { (yyval.regno) = 2; }
    break;

  case 272:
#line 1106 "rl78-parse.y"
    { (yyval.regno) = 3; }
    break;

  case 273:
#line 1107 "rl78-parse.y"
    { (yyval.regno) = 4; }
    break;

  case 274:
#line 1108 "rl78-parse.y"
    { (yyval.regno) = 5; }
    break;

  case 275:
#line 1109 "rl78-parse.y"
    { (yyval.regno) = 6; }
    break;

  case 276:
#line 1110 "rl78-parse.y"
    { (yyval.regno) = 7; }
    break;

  case 277:
#line 1113 "rl78-parse.y"
    { (yyval.regno) = 0; }
    break;

  case 278:
#line 1114 "rl78-parse.y"
    { (yyval.regno) = 1; }
    break;

  case 279:
#line 1115 "rl78-parse.y"
    { (yyval.regno) = 2; }
    break;

  case 280:
#line 1116 "rl78-parse.y"
    { (yyval.regno) = 3; }
    break;

  case 281:
#line 1119 "rl78-parse.y"
    { (yyval.regno) = 1; }
    break;

  case 282:
#line 1120 "rl78-parse.y"
    { (yyval.regno) = 2; }
    break;

  case 283:
#line 1121 "rl78-parse.y"
    { (yyval.regno) = 3; }
    break;

  case 284:
#line 1124 "rl78-parse.y"
    { (yyval.regno) = 0xf8; }
    break;

  case 285:
#line 1125 "rl78-parse.y"
    { (yyval.regno) = 0xf9; }
    break;

  case 286:
#line 1126 "rl78-parse.y"
    { (yyval.regno) = 0xfa; }
    break;

  case 287:
#line 1127 "rl78-parse.y"
    { (yyval.regno) = 0xfc; }
    break;

  case 288:
#line 1128 "rl78-parse.y"
    { (yyval.regno) = 0xfd; }
    break;

  case 289:
#line 1129 "rl78-parse.y"
    { (yyval.regno) = 0xfe; }
    break;

  case 290:
#line 1130 "rl78-parse.y"
    { (yyval.regno) = 0xff; }
    break;

  case 291:
#line 1136 "rl78-parse.y"
    { (yyval.regno) = 0x00; }
    break;

  case 292:
#line 1137 "rl78-parse.y"
    { (yyval.regno) = 0x10; }
    break;

  case 293:
#line 1138 "rl78-parse.y"
    { (yyval.regno) = 0x20; }
    break;

  case 294:
#line 1139 "rl78-parse.y"
    { (yyval.regno) = 0x30; }
    break;

  case 295:
#line 1140 "rl78-parse.y"
    { (yyval.regno) = 0x40; }
    break;

  case 296:
#line 1141 "rl78-parse.y"
    { (yyval.regno) = 0x50; }
    break;

  case 297:
#line 1142 "rl78-parse.y"
    { (yyval.regno) = 0x60; }
    break;

  case 298:
#line 1143 "rl78-parse.y"
    { (yyval.regno) = 0x70; }
    break;

  case 299:
#line 1146 "rl78-parse.y"
    { (yyval.regno) = 0x00; }
    break;

  case 300:
#line 1147 "rl78-parse.y"
    { (yyval.regno) = 0x20; }
    break;

  case 301:
#line 1148 "rl78-parse.y"
    { (yyval.regno) = 0x40; }
    break;

  case 302:
#line 1151 "rl78-parse.y"
    { (yyval.regno) = 0x05; rl78_bit_insn = 1; }
    break;

  case 303:
#line 1152 "rl78-parse.y"
    { (yyval.regno) = 0x06; rl78_bit_insn = 1;}
    break;

  case 304:
#line 1153 "rl78-parse.y"
    { (yyval.regno) = 0x07; rl78_bit_insn = 1; }
    break;

  case 305:
#line 1156 "rl78-parse.y"
    { (yyval.regno) = 0x02;    rl78_bit_insn = 1;}
    break;

  case 306:
#line 1157 "rl78-parse.y"
    { (yyval.regno) = 0x04;    rl78_bit_insn = 1; }
    break;

  case 307:
#line 1158 "rl78-parse.y"
    { (yyval.regno) = 0x00; rl78_bit_insn = 1; }
    break;

  case 308:
#line 1161 "rl78-parse.y"
    { (yyval.regno) = 0; rl78_bit_insn = 1; }
    break;

  case 309:
#line 1162 "rl78-parse.y"
    { (yyval.regno) = 1; rl78_bit_insn = 1; }
    break;

  case 310:
#line 1165 "rl78-parse.y"
    { (yyval.regno) = 0x00; }
    break;

  case 311:
#line 1166 "rl78-parse.y"
    { (yyval.regno) = 0x10; }
    break;

  case 312:
#line 1169 "rl78-parse.y"
    { (yyval.regno) = 0x00; }
    break;

  case 313:
#line 1170 "rl78-parse.y"
    { (yyval.regno) = 0x10; }
    break;

  case 314:
#line 1173 "rl78-parse.y"
    { (yyval.regno) = 0x00; }
    break;

  case 315:
#line 1174 "rl78-parse.y"
    { (yyval.regno) = 0x10; }
    break;

  case 316:
#line 1177 "rl78-parse.y"
    { (yyval.regno) = 0x00; }
    break;

  case 317:
#line 1178 "rl78-parse.y"
    { (yyval.regno) = 0x10; }
    break;

  case 318:
#line 1181 "rl78-parse.y"
    { rl78_bit_insn = 1; }
    break;


/* Line 1267 of yacc.c.  */
#line 4091 "rl78-parse.c"
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


#line 1184 "rl78-parse.y"

/* ====================================================================== */

static struct
{
  const char * string;
  int          token;
  int          val;
}
token_table[] =
{
  { "r0", X, 0 },
  { "r1", A, 1 },
  { "r2", C, 2 },
  { "r3", B, 3 },
  { "r4", E, 4 },
  { "r5", D, 5 },
  { "r6", L, 6 },
  { "r7", H, 7 },
  { "x", X, 0 },
  { "a", A, 1 },
  { "c", C, 2 },
  { "b", B, 3 },
  { "e", E, 4 },
  { "d", D, 5 },
  { "l", L, 6 },
  { "h", H, 7 },

  { "rp0", AX, 0 },
  { "rp1", BC, 1 },
  { "rp2", DE, 2 },
  { "rp3", HL, 3 },
  { "ax", AX, 0 },
  { "bc", BC, 1 },
  { "de", DE, 2 },
  { "hl", HL, 3 },

  { "RB0", RB0, 0 },
  { "RB1", RB1, 1 },
  { "RB2", RB2, 2 },
  { "RB3", RB3, 3 },

  { "sp", SP, 0 },
  { "cy", CY, 0 },

  { "spl", SPL, 0xf8 },
  { "sph", SPH, 0xf9 },
  { "psw", PSW, 0xfa },
  { "cs", CS, 0xfc },
  { "es", ES, 0xfd },
  { "pmc", PMC, 0xfe },
  { "mem", MEM, 0xff },

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

  OPC(ADD),
  OPC(ADDC),
  OPC(ADDW),
  { "and", AND_, IS_OPCODE },
  OPC(AND1),
  OPC(BC),
  OPC(BF),
  OPC(BH),
  OPC(BNC),
  OPC(BNH),
  OPC(BNZ),
  OPC(BR),
  OPC(BRK),
  OPC(BRK1),
  OPC(BT),
  OPC(BTCLR),
  OPC(BZ),
  OPC(CALL),
  OPC(CALLT),
  OPC(CLR1),
  OPC(CLRB),
  OPC(CLRW),
  OPC(CMP),
  OPC(CMP0),
  OPC(CMPS),
  OPC(CMPW),
  OPC(DEC),
  OPC(DECW),
  OPC(DI),
  OPC(DIVHU),
  OPC(DIVWU),
  OPC(EI),
  OPC(HALT),
  OPC(INC),
  OPC(INCW),
  OPC(MACH),
  OPC(MACHU),
  OPC(MOV),
  OPC(MOV1),
  OPC(MOVS),
  OPC(MOVW),
  OPC(MULH),
  OPC(MULHU),
  OPC(MULU),
  OPC(NOP),
  OPC(NOT1),
  OPC(ONEB),
  OPC(ONEW),
  OPC(OR),
  OPC(OR1),
  OPC(POP),
  OPC(PUSH),
  OPC(RET),
  OPC(RETI),
  OPC(RETB),
  OPC(ROL),
  OPC(ROLC),
  OPC(ROLWC),
  OPC(ROR),
  OPC(RORC),
  OPC(SAR),
  OPC(SARW),
  OPC(SEL),
  OPC(SET1),
  OPC(SHL),
  OPC(SHLW),
  OPC(SHR),
  OPC(SHRW),
  OPC(SKC),
  OPC(SKH),
  OPC(SKNC),
  OPC(SKNH),
  OPC(SKNZ),
  OPC(SKZ),
  OPC(STOP),
  OPC(SUB),
  OPC(SUBC),
  OPC(SUBW),
  OPC(XCH),
  OPC(XCHW),
  OPC(XOR),
  OPC(XOR1),
};

#define NUM_TOKENS (sizeof (token_table) / sizeof (token_table[0]))

void
rl78_lex_init (char * beginning, char * ending)
{
  rl78_init_start = beginning;
  rl78_lex_start = beginning;
  rl78_lex_end = ending;
  rl78_in_brackets = 0;
  rl78_last_token = 0;

  rl78_bit_insn = 0;

  setbuf (stdout, 0);
}

/* Return a pointer to the '.' in a bit index expression (like
   foo.5), or NULL if none is found.  */
static char *
find_bit_index (char *tok)
{
  char *last_dot = NULL;
  char *last_digit = NULL;
  while (*tok && *tok != ',')
    {
      if (*tok == '.')
	{
	  last_dot = tok;
	  last_digit = NULL;
	}
      else if (*tok >= '0' && *tok <= '7'
	       && last_dot != NULL
	       && last_digit == NULL)
	{
	  last_digit = tok;
	}
      else if (ISSPACE (*tok))
	{
	  /* skip */
	}
      else
	{
	  last_dot = NULL;
	  last_digit = NULL;
	}
      tok ++;
    }
  if (last_dot != NULL
      && last_digit != NULL)
    return last_dot;
  return NULL;
}

static int
rl78_lex (void)
{
  /*unsigned int ci;*/
  char * save_input_pointer;
  char * bit = NULL;

  while (ISSPACE (*rl78_lex_start)
	 && rl78_lex_start != rl78_lex_end)
    rl78_lex_start ++;

  rl78_last_exp_start = rl78_lex_start;

  if (rl78_lex_start == rl78_lex_end)
    return 0;

  if (ISALPHA (*rl78_lex_start)
      || (*rl78_lex_start == '.' && ISALPHA (rl78_lex_start[1])))
    {
      unsigned int i;
      char * e;
      char save;

      for (e = rl78_lex_start + 1;
	   e < rl78_lex_end && ISALNUM (*e);
	   e ++)
	;
      save = *e;
      *e = 0;

      for (i = 0; i < NUM_TOKENS; i++)
	if (strcasecmp (rl78_lex_start, token_table[i].string) == 0
	    && !(token_table[i].val == IS_OPCODE && rl78_last_token != 0)
	    && !(token_table[i].token == FLAG && !need_flag))
	  {
	    rl78_lval.regno = token_table[i].val;
	    *e = save;
	    rl78_lex_start = e;
	    rl78_last_token = token_table[i].token;
	    return token_table[i].token;
	  }
      *e = save;
    }

  if (rl78_last_token == 0)
    {
      rl78_last_token = UNKNOWN_OPCODE;
      return UNKNOWN_OPCODE;
    }

  if (rl78_last_token == UNKNOWN_OPCODE)
    return 0;

  if (*rl78_lex_start == '[')
    rl78_in_brackets = 1;
  if (*rl78_lex_start == ']')
    rl78_in_brackets = 0;

  /* '.' is funny - the syntax includes it for bitfields, but only for
      bitfields.  We check for it specially so we can allow labels
      with '.' in them.  */

  if (rl78_bit_insn
      && *rl78_lex_start == '.'
      && find_bit_index (rl78_lex_start) == rl78_lex_start)
    {
      rl78_last_token = *rl78_lex_start;
      return *rl78_lex_start ++;
    }

  if ((rl78_in_brackets && *rl78_lex_start == '+')
      || strchr ("[],#!$:", *rl78_lex_start))
    {
      rl78_last_token = *rl78_lex_start;
      return *rl78_lex_start ++;
    }

  /* Again, '.' is funny.  Look for '.<digit>' at the end of the line
     or before a comma, which is a bitfield, not an expression.  */

  if (rl78_bit_insn)
    {
      bit = find_bit_index (rl78_lex_start);
      if (bit)
	*bit = 0;
      else
	bit = NULL;
    }

  save_input_pointer = input_line_pointer;
  input_line_pointer = rl78_lex_start;
  rl78_lval.exp.X_md = 0;
  expression (&rl78_lval.exp);

  if (bit)
    *bit = '.';

  rl78_lex_start = input_line_pointer;
  input_line_pointer = save_input_pointer;
  rl78_last_token = EXPR;
  return EXPR;
}

int
rl78_error (const char * str)
{
  int len;

  len = rl78_last_exp_start - rl78_init_start;

  as_bad ("%s", rl78_init_start);
  as_bad ("%*s^ %s", len, "", str);
  return 0;
}

static int
expr_is_sfr (expressionS exp)
{
  unsigned long v;

  if (exp.X_op != O_constant)
    return 0;

  v = exp.X_add_number;
  if (0xFFF00 <= v && v <= 0xFFFFF)
    return 1;
  return 0;
}

static int
expr_is_saddr (expressionS exp)
{
  unsigned long v;

  if (exp.X_op != O_constant)
    return 0;

  v = exp.X_add_number;
  if (0xFFE20 <= v && v <= 0xFFF1F)
    return 1;
  return 0;
}

static int
expr_is_word_aligned (expressionS exp)
{
  unsigned long v;

  if (exp.X_op != O_constant)
    return 1;

  v = exp.X_add_number;
  if (v & 1)
    return 0;
  return 1;
  
}

static void
check_expr_is_bit_index (expressionS exp)
{
  int val;

  if (exp.X_op != O_constant)
    {
      rl78_error (_("bit index must be a constant"));
      return;
    }
  val = exp.X_add_number;

  if (val < 0 || val > 7)
    rl78_error (_("rtsd size must be 0..7"));
}

static int
exp_val (expressionS exp)
{
  if (exp.X_op != O_constant)
  {
    rl78_error (_("constant expected"));
    return 0;
  }
  return exp.X_add_number;
}

static int
check_expr_is_const (expressionS e, int vmin, int vmax)
{
  static char buf[100];
  if (e.X_op != O_constant
      || e.X_add_number < vmin
      || e.X_add_number > vmax)
    {
      if (vmin == vmax)
	sprintf (buf, "%d expected here", vmin);
      else
	sprintf (buf, "%d..%d expected here", vmin, vmax);
      rl78_error(buf);
      return 0;
    }
  return 1;
}



