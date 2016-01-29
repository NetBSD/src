/* A Bison parser, made by GNU Bison 3.0.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2013 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

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
#define YYBISON_VERSION "3.0"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1


/* Substitute the variable and function names.  */
#define yyparse         rx_parse
#define yylex           rx_lex
#define yyerror         rx_error
#define yydebug         rx_debug
#define yynerrs         rx_nerrs

#define yylval          rx_lval
#define yychar          rx_char

/* Copy the first part of user declarations.  */
#line 20 "./config/rx-parse.y" /* yacc.c:339  */


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

static void	   rx_check_float_support (void);
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


#line 186 "rx-parse.c" /* yacc.c:339  */

# ifndef YY_NULL
#  if defined __cplusplus && 201103L <= __cplusplus
#   define YY_NULL nullptr
#  else
#   define YY_NULL 0
#  endif
# endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* In a future release of Bison, this section will be replaced
   by #include "y.tab.h".  */
#ifndef YY_RX_RX_PARSE_H_INCLUDED
# define YY_RX_RX_PARSE_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int rx_debug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
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

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE YYSTYPE;
union YYSTYPE
{
#line 134 "./config/rx-parse.y" /* yacc.c:355  */

  int regno;
  expressionS exp;

#line 459 "rx-parse.c" /* yacc.c:355  */
};
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE rx_lval;

int rx_parse (void);

#endif /* !YY_RX_RX_PARSE_H_INCLUDED  */

/* Copy the second part of user declarations.  */

#line 474 "rx-parse.c" /* yacc.c:358  */

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
#else
typedef signed char yytype_int8;
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
# elif ! defined YYSIZE_T
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
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif

#ifndef __attribute__
/* This feature is available in gcc versions 2.5 and later.  */
# if (! defined __GNUC__ || __GNUC__ < 2 \
      || (__GNUC__ == 2 && __GNUC_MINOR__ < 5))
#  define __attribute__(Spec) /* empty */
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif

#if defined __GNUC__ && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN \
    _Pragma ("GCC diagnostic push") \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")\
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# define YY_IGNORE_MAYBE_UNINITIALIZED_END \
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
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
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
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
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
        YYSIZE_T yynewbytes;                                            \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / sizeof (*yyptr);                          \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, (Count) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYSIZE_T yyi;                         \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  216
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   627

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  121
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  63
/* YYNRULES -- Number of rules.  */
#define YYNRULES  246
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  611

/* YYTRANSLATE[YYX] -- Symbol number corresponding to YYX as returned
   by yylex, with out-of-bounds checking.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   369

#define YYTRANSLATE(YYX)                                                \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, without out-of-bounds checking.  */
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
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
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
     885,   885,   888,   892,   892,   895,   895,   902,   903,   906,
     906,   911,   912,   913,   914,   915,   918,   919,   920,   921,
     924,   925,   926,   929,   930,   933,   934
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
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
  "']'", "'-'", "'+'", "$accept", "statement", "$@1", "$@2", "$@3", "$@4",
  "$@5", "$@6", "$@7", "$@8", "$@9", "$@10", "$@11", "$@12", "$@13",
  "$@14", "$@15", "$@16", "$@17", "$@18", "$@19", "$@20", "$@21", "$@22",
  "$@23", "$@24", "$@25", "$@26", "$@27", "$@28", "$@29", "$@30", "$@31",
  "$@32", "$@33", "$@34", "$@35", "$@36", "$@37", "$@38", "op_subadd",
  "op_dp20_rm_l", "op_dp20_rm", "op_dp20_i", "op_dp20_rim",
  "op_dp20_rim_l", "op_dp20_rr", "op_xchg", "op_shift_rot", "op_shift",
  "float2_op", "$@39", "float2_op_ni", "$@40", "$@41", "disp", "flag",
  "$@42", "memex", "bwl", "bw", "opt_l", "opt_b", YY_NULL
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
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

#define YYPACT_NINF -483

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-483)))

#define YYTABLE_NINF -224

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     504,  -483,  -483,  -483,   -78,   -55,    16,   141,   -53,    18,
      34,  -483,    19,    75,    20,  -483,     1,  -483,  -483,  -483,
     -49,   -32,  -483,  -483,  -483,  -483,  -483,  -483,   -30,  -483,
      51,    92,   127,   145,  -483,  -483,    17,    79,    45,   146,
     158,   177,   193,   194,   188,   196,   200,    21,    89,  -483,
    -483,  -483,    90,   203,   202,   205,    67,   204,   207,    96,
    -483,  -483,    67,   209,   210,    99,   101,  -483,  -483,  -483,
    -483,   102,   215,  -483,   104,   190,  -483,  -483,  -483,  -483,
    -483,  -483,  -483,  -483,    67,  -483,  -483,   105,    67,    67,
    -483,  -483,  -483,  -483,   222,   220,    12,   218,    47,   219,
      47,   110,   221,  -483,   223,   224,   225,   226,   118,   229,
    -483,   230,   231,   232,  -483,   233,   237,   125,   236,  -483,
     238,   239,   240,   130,   241,  -483,   244,   133,  -483,   245,
     135,    14,    14,   138,    53,   138,    53,    22,    22,    22,
      22,    22,   247,   248,    53,  -483,  -483,   140,   142,    14,
      14,   144,   147,   148,   251,     6,  -483,  -483,     7,   254,
      47,   149,   150,  -483,  -483,  -483,   151,  -483,  -483,   152,
     255,   258,   220,   220,   263,    47,  -483,  -483,   153,  -483,
    -483,  -483,    64,  -483,   154,   264,   268,   268,  -483,  -483,
    -483,   269,   268,   270,   268,   247,   271,  -483,   272,    98,
     276,   274,  -483,    23,    23,    23,  -483,   138,   138,   275,
      47,  -483,  -483,    14,    53,    14,  -483,   167,  -483,   168,
     279,  -483,  -483,  -483,   157,   173,   174,  -483,   169,   175,
    -483,   164,   179,  -483,  -483,  -483,   180,   165,   181,  -483,
    -483,  -483,  -483,  -483,   166,   182,  -483,  -483,  -483,   170,
     186,  -483,   284,   187,   285,   191,  -483,  -483,  -483,   192,
    -483,  -483,   195,  -483,   197,  -483,  -483,  -483,   198,  -483,
     289,   274,  -483,  -483,  -483,  -483,  -483,  -483,  -483,   290,
     291,  -483,  -483,   293,   294,   295,   199,   201,   206,     0,
     208,   211,     2,   216,  -483,   301,   302,   303,   277,   227,
    -483,  -483,  -483,   228,  -483,   305,  -483,   214,   307,  -483,
     234,  -483,  -483,   235,  -483,   242,  -483,  -483,   243,   246,
    -483,  -483,   217,   306,  -483,  -483,  -483,  -483,  -483,  -483,
     249,  -483,  -483,  -483,  -483,   313,   315,   250,   316,   317,
     318,   319,   322,  -483,   252,   171,   172,  -483,   253,   176,
    -483,   256,   178,  -483,   257,   183,  -483,   323,   259,   324,
     332,   334,   335,   336,   260,   261,  -483,  -483,   265,   266,
     267,   337,     8,   338,   -45,   342,   343,   344,    37,   345,
     346,  -483,  -483,  -483,  -483,   347,   350,  -483,   351,  -483,
     352,   353,   354,   357,   358,   360,   273,   361,  -483,  -483,
     364,   262,   278,   280,   281,  -483,   365,  -483,   283,  -483,
     286,   368,  -483,   287,   369,  -483,   288,   372,  -483,   292,
    -483,    82,  -483,   298,  -483,   299,   282,   376,   381,   274,
     274,   184,  -483,  -483,   304,     3,   300,   382,   296,   308,
     309,  -483,   310,   383,   314,   311,   320,  -483,  -483,  -483,
     321,  -483,  -483,  -483,   312,  -483,   325,   384,  -483,  -483,
     378,   385,   388,    88,   326,   389,   390,   327,   392,   328,
     394,   329,   398,  -483,  -483,  -483,   333,  -483,   339,    95,
     131,   399,  -483,   340,   348,   349,  -483,   355,   403,    38,
     404,   341,   356,   185,   359,   362,   363,   366,   405,   367,
     370,  -483,   407,  -483,   371,   373,  -483,  -483,   374,   375,
     401,   377,   379,   401,   380,   401,   386,   401,   387,   416,
     417,   391,   393,   396,   397,  -483,   378,   419,   420,   421,
     400,   422,  -483,   428,   444,   189,   423,  -483,   462,   429,
     430,   431,   467,  -483,   432,   433,  -483,   434,   437,   438,
     439,  -483,  -483,   440,   401,  -483,   401,  -483,   441,  -483,
     442,  -483,  -483,   450,   451,   453,   456,   494,   501,   502,
     503,  -483,   505,  -483,  -483,  -483,   507,   457,   458,  -483,
    -483,  -483,   459,  -483,  -483,  -483,  -483,  -483,  -483,  -483,
    -483,  -483,  -483,  -483,  -483,  -483,  -483,  -483,   460,  -483,
    -483,  -483,  -483,   461,  -483,   508,  -483,  -483,   509,  -483,
    -483
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,     2,    94,    92,   188,   192,     0,     0,     0,     0,
     243,     3,     0,   243,     0,   229,   227,     4,   104,   106,
     118,   120,   138,   136,   142,   140,   134,   144,     0,   124,
       0,     0,     0,     0,    96,    98,   236,   240,   190,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    90,
       6,   112,   194,     0,     0,     0,   236,     0,     0,     0,
     174,   172,   236,     0,     0,   168,   170,   146,    73,    72,
       5,     0,     0,    75,    88,   236,    64,   229,    40,    42,
      38,    66,    67,    65,   236,   116,   114,   186,   236,   236,
     108,    74,   122,   110,     0,     0,   227,     0,   227,     0,
     227,     0,     0,    18,     0,     0,     0,     0,     0,     0,
       7,     0,     0,     0,   244,     0,     0,     0,     0,    10,
       0,     0,     0,     0,     0,    59,     0,     0,   228,     0,
       0,   227,   227,     0,   227,     0,   227,   225,   225,   225,
     225,   225,   225,     0,   227,    60,    61,     0,     0,   227,
     227,   237,   238,   239,     0,     0,   241,   242,     0,     0,
     227,     0,     0,   158,   159,   160,     0,   156,   157,     0,
       0,     0,     0,     0,     0,   227,    55,    57,     0,   237,
     238,   239,   227,    56,     0,     0,     0,     0,    71,    53,
      52,     0,     0,     0,     0,   225,     0,    51,     0,   227,
     239,   227,    58,     0,     0,     0,    70,     0,     0,     0,
     227,    68,    69,   227,   227,   227,     1,   212,    95,     0,
       0,   209,   210,    93,     0,     0,     0,   189,     0,     0,
     193,   227,     0,    12,    13,    17,     0,   227,     0,     9,
      14,    15,     8,    62,   227,     0,    16,    11,    63,   227,
       0,   230,     0,     0,     0,     0,   207,   208,   105,     0,
     107,   101,     0,   119,     0,   103,   121,   139,     0,   222,
       0,   227,   137,   143,   141,   135,   145,    47,   125,     0,
       0,    97,    99,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   191,     0,     0,     0,     0,     0,
      76,    91,   113,     0,   195,     0,    54,     0,     0,   161,
       0,   175,   173,     0,   169,     0,   171,   147,    37,     0,
      89,   148,     0,     0,   219,    41,    43,    39,   117,   115,
       0,   187,   109,   123,   111,     0,     0,     0,     0,     0,
       0,     0,     0,   127,     0,   227,   227,   129,     0,   227,
     126,     0,   227,   128,     0,   227,    23,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   154,   155,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   152,   153,   177,   176,     0,     0,    45,     0,    44,
       0,     0,     0,     0,     0,     0,     0,     0,   211,   201,
       0,     0,    31,   197,     0,    33,     0,    49,     0,   181,
       0,     0,   182,     0,     0,    48,     0,     0,    50,     0,
      30,   233,   203,     0,   213,     0,     0,     0,     0,   227,
     227,   227,    36,    82,     0,     0,     0,     0,     0,     0,
       0,    26,     0,     0,     0,     0,     0,    32,   180,    34,
       0,   216,   178,   179,     0,   196,     0,     0,    29,   206,
     243,     0,     0,   233,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   231,   232,   234,     0,   235,     0,   233,
     233,     0,   224,     0,     0,     0,    35,     0,     0,     0,
       0,     0,     0,   227,     0,     0,     0,     0,     0,     0,
       0,    87,     0,   149,   217,     0,    46,   200,     0,     0,
     245,     0,     0,   245,     0,   245,     0,   245,     0,     0,
       0,     0,     0,     0,     0,   221,   243,     0,     0,     0,
       0,     0,    79,     0,     0,   227,     0,    83,     0,     0,
       0,     0,     0,    27,     0,     0,    22,     0,     0,     0,
       0,   246,   131,     0,   245,   133,   245,   130,     0,   132,
       0,    24,    25,     0,     0,     0,     0,     0,     0,     0,
       0,    77,     0,   162,   163,    78,     0,     0,     0,   164,
     165,    28,     0,   166,   167,   218,   202,   198,   199,    85,
     150,   151,    84,    86,   204,   205,   214,   215,     0,    19,
      20,    21,   183,     0,   184,     0,   185,   226,     0,    80,
      81
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -483,  -483,  -483,  -483,  -483,  -483,  -483,  -483,  -483,  -483,
    -483,  -483,  -483,  -483,  -483,  -483,  -483,  -483,  -483,  -483,
    -483,  -483,  -483,  -483,  -483,  -483,  -483,  -483,  -483,  -483,
    -483,  -483,  -483,  -483,  -483,  -483,  -483,  -483,  -483,  -483,
     -84,   297,  -483,   -94,   -98,  -483,   -61,  -106,  -129,   -65,
      24,  -483,  -134,  -483,  -483,   -16,   408,  -483,  -431,   -20,
    -483,   -12,  -482
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    94,   205,   203,   204,   199,   172,    96,    95,   149,
     150,   133,   135,   131,   132,   213,   215,   173,   208,   207,
     134,   136,   214,   144,   141,   138,   137,   140,   139,   142,
     195,   192,   194,   187,   186,   210,    98,   160,   100,   175,
     227,   221,   256,   257,   258,   223,   218,   263,   324,   325,
     267,   268,   269,   270,   271,   259,   125,   126,   478,   155,
     158,   116,   552
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
     130,   122,   222,   374,   127,   378,   489,   128,   276,   287,
     290,   433,   288,   291,   434,   219,   230,   255,   128,   101,
     128,   108,   117,   123,   169,  -223,   310,   151,   152,   153,
     266,   555,   509,   557,   260,   559,   182,    97,   278,   261,
     110,   265,   188,   111,   112,   113,   114,   115,   522,   524,
     226,   281,   282,   128,   145,   201,   262,   311,   312,   128,
      99,   317,   107,   314,   206,   316,  -100,   306,   211,   212,
     128,   437,   590,   438,   591,   439,   294,   179,   180,   181,
     224,   119,   228,  -102,   228,   143,   120,   114,   121,   156,
     157,   304,   473,   474,   475,   146,   476,   477,   473,   474,
     475,   219,   508,   477,   128,   473,   474,   475,   333,   521,
     477,   301,   302,   328,   329,   332,   129,   334,   264,   375,
     264,   379,   490,   289,   292,   435,   331,   220,   264,   220,
     147,   102,   154,   109,   118,   124,   170,  -220,   323,   326,
     327,   473,   474,   475,   228,   523,   477,   103,   148,   161,
     104,   105,   106,   443,   531,   444,   532,   445,   533,   228,
     159,   162,   272,   273,   274,   275,   307,   343,   347,   350,
     128,   128,   128,   353,   407,   409,   128,   128,   128,   412,
     163,   415,   128,   224,   128,   322,   418,   486,   537,   128,
     128,   128,   575,   166,   228,   128,   164,   165,   264,   167,
     179,   180,   200,   168,   171,   174,   176,   177,   178,   183,
     184,   185,   189,   190,   191,   344,   193,   196,   197,   198,
     209,   348,   216,   217,   225,   229,   231,   232,   351,   233,
     234,   235,   236,   354,   237,   238,   239,   240,   241,   242,
     243,   244,   245,   248,   246,   247,   249,   250,   251,   252,
    -223,   253,   254,   220,   277,   365,   279,   286,   280,   283,
     293,   299,   284,   285,   300,   295,   296,   297,   298,   303,
     309,   310,   305,   308,   338,   313,   315,   318,   319,   321,
     128,   330,   384,   335,   336,   337,   341,   356,   358,   339,
     340,   342,   364,   366,   367,   345,   346,   349,   352,   368,
     369,   370,   355,   357,   381,   382,   383,   359,   387,   360,
     389,   361,   396,   363,   362,   371,   398,   372,   399,   401,
     402,   403,   404,   373,   376,   405,   420,   422,   377,   408,
     410,   388,   380,   413,   395,   423,   416,   424,   425,   419,
     432,   436,   426,   385,   386,   440,   441,   442,   446,   447,
     390,   391,   448,   449,   450,   451,   452,   453,   392,   393,
     454,   455,   394,   456,   458,   397,   400,   459,   464,   406,
     411,   467,   469,   414,   417,   471,   427,   421,   428,   482,
     460,   429,   430,   431,   483,   492,   497,   504,   506,   457,
     114,   507,   511,   512,   461,   514,   462,   516,   481,   463,
     465,   518,   525,   466,   468,   470,   530,   534,   543,   472,
     546,   551,   493,   484,   485,   487,   479,   480,   491,   561,
     562,   488,   568,   569,   570,   572,   494,   495,   496,   499,
     498,   502,   579,   580,   581,   583,   584,   585,   500,   501,
     586,   587,   588,   503,   510,   513,   515,   517,   505,   519,
     589,   592,   593,   594,   595,   520,   596,   535,   526,   597,
     604,   605,   606,   607,   608,   527,   528,     0,     0,     0,
       0,     0,   529,     0,   536,   539,     0,   538,   540,   541,
       0,     0,     0,   544,   542,   202,   545,   547,     0,   548,
     549,   550,     0,     0,     0,   553,   320,   554,   556,     0,
       0,     0,     0,     0,   558,   560,     0,   563,     0,   564,
       0,     1,   565,   566,   567,     0,     0,     0,   571,   576,
       2,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,     0,   577,
      20,    21,    22,    23,    24,    25,   573,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    40,   574,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,   578,
      57,    58,    59,   582,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
     598,    86,    87,    88,    89,    90,    91,    92,    93,   599,
     600,   601,     0,   602,   603,     0,   609,   610
};

static const yytype_int16 yycheck[] =
{
      16,    13,    96,     3,     3,     3,     3,     6,   142,     3,
       3,     3,     6,     6,     6,     3,   100,     3,     6,     3,
       6,     3,     3,     3,     3,     3,     3,    10,    11,    12,
     136,   513,   463,   515,   132,   517,    56,   115,   144,   133,
       6,   135,    62,     9,    10,    11,    12,    13,   479,   480,
       3,   149,   150,     6,     3,    75,     3,   186,   187,     6,
     115,   195,   115,   192,    84,   194,   115,     3,    88,    89,
       6,   116,   554,   118,   556,   120,   160,    10,    11,    12,
      96,     6,    98,   115,   100,   115,    11,    12,    13,    10,
      11,   175,    10,    11,    12,     3,    14,    15,    10,    11,
      12,     3,    14,    15,     6,    10,    11,    12,   214,    14,
      15,   172,   173,   207,   208,   213,   115,   215,   134,   119,
     136,   119,   119,   117,   117,   117,   210,   115,   144,   115,
       3,   115,   115,   115,   115,   115,   115,   115,   115,   204,
     205,    10,    11,    12,   160,    14,    15,     6,     3,     3,
       9,    10,    11,   116,   116,   118,   118,   120,   120,   175,
     115,     3,   138,   139,   140,   141,   182,     3,     3,     3,
       6,     6,     6,     3,     3,     3,     6,     6,     6,     3,
       3,     3,     6,   199,     6,   201,     3,     3,     3,     6,
       6,     6,     3,     5,   210,     6,     3,     3,   214,     3,
      10,    11,    12,     3,   115,   115,     3,     5,     3,     5,
       3,   115,     3,     3,   115,   231,   115,   115,     3,   115,
     115,   237,     0,     3,     6,     6,   116,     6,   244,     6,
       6,     6,     6,   249,   116,     6,     6,     6,     6,     6,
       3,   116,     6,     3,     6,     6,   116,     6,     4,   116,
       3,     6,   117,   115,     6,   271,   116,     6,   116,   115,
       6,     6,   115,   115,     6,   116,   116,   116,   116,     6,
       6,     3,   119,   119,   117,     6,     6,     6,     6,     3,
       6,     6,     5,   116,   116,     6,   117,     3,     3,   116,
     116,   116,     3,     3,     3,   116,   116,   116,   116,     6,
       6,     6,   116,   116,     3,     3,     3,   116,     3,   117,
       3,   116,     6,   115,   117,   116,     3,   116,     3,     3,
       3,     3,     3,   117,   116,     3,     3,     3,   117,   345,
     346,   117,   116,   349,   117,     3,   352,     3,     3,   355,
       3,     3,     6,   116,   116,     3,     3,     3,     3,     3,
     116,   116,     5,     3,     3,     3,     3,     3,   116,   116,
       3,     3,   116,     3,     3,   116,   116,     3,     3,   117,
     117,     3,     3,   117,   117,     3,   116,   118,   117,     3,
     118,   116,   116,   116,     3,     3,     3,     3,     3,   116,
      12,     3,     3,     3,   116,     3,   116,     3,   116,   118,
     117,     3,     3,   117,   117,   117,     3,     3,     3,   117,
       3,    10,   116,   429,   430,   431,   118,   118,   118,     3,
       3,   117,     3,     3,     3,     3,   118,   118,   118,   118,
     116,   119,     3,     3,     3,     3,     3,     3,   118,   118,
       3,     3,     3,   118,   118,   118,   118,   118,   460,   116,
      10,    10,    10,     3,     3,   116,     3,   116,   118,     3,
       3,     3,     3,     3,     3,   117,   117,    -1,    -1,    -1,
      -1,    -1,   117,    -1,   118,   116,    -1,   493,   116,   116,
      -1,    -1,    -1,   116,   118,    77,   116,   116,    -1,   116,
     116,   116,    -1,    -1,    -1,   118,   199,   118,   118,    -1,
      -1,    -1,    -1,    -1,   118,   118,    -1,   116,    -1,   116,
      -1,     7,   116,   116,   526,    -1,    -1,    -1,   118,   535,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    -1,   116,
      36,    37,    38,    39,    40,    41,   118,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,   118,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,   117,
      76,    77,    78,   116,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    98,    99,   100,   101,   102,   103,   104,   105,
     116,   107,   108,   109,   110,   111,   112,   113,   114,   118,
     118,   118,    -1,   118,   117,    -1,   118,   118
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
       6,     9,    10,    11,    12,    13,   182,     3,   115,     6,
      11,    13,   182,     3,   115,   177,   178,     3,     6,   115,
     176,   134,   135,   132,   141,   133,   142,   147,   146,   149,
     148,   145,   150,   115,   144,     3,     3,     3,     3,   130,
     131,    10,    11,    12,   115,   180,    10,    11,   181,   115,
     158,     3,     3,     3,     3,     3,     5,     3,     3,     3,
     115,   115,   127,   138,   115,   160,     3,     5,     3,    10,
      11,    12,   180,     5,     3,   115,   155,   154,   180,     3,
       3,   115,   152,   115,   153,   151,   115,     3,   115,   126,
      12,   180,   177,   124,   125,   123,   180,   140,   139,   115,
     156,   180,   180,   136,   143,   137,     0,     3,   167,     3,
     115,   162,   164,   166,   176,     6,     3,   161,   176,     6,
     161,   116,     6,     6,     6,     6,     6,   116,     6,     6,
       6,     6,     6,     3,   116,     6,     6,     6,     3,   116,
       6,     4,   116,     6,   117,     3,   163,   164,   165,   176,
     165,   164,     3,   168,   176,   164,   168,   171,   172,   173,
     174,   175,   171,   171,   171,   171,   173,     6,   168,   116,
     116,   165,   165,   115,   115,   115,     6,     3,     6,   117,
       3,     6,   117,     6,   161,   116,   116,   116,   116,     6,
       6,   167,   167,     6,   161,   119,     3,   176,   119,     6,
       3,   169,   169,     6,   169,     6,   169,   173,     6,     6,
     162,     3,   176,   115,   169,   170,   170,   170,   164,   164,
       6,   161,   165,   168,   165,   116,   116,     6,   117,   116,
     116,   117,   116,     3,   176,   116,   116,     3,   176,   116,
       3,   176,   116,     3,   176,   116,     3,   116,     3,   116,
     117,   116,   117,   115,     3,   176,     3,     3,     6,     6,
       6,   116,   116,   117,     3,   119,   116,   117,     3,   119,
     116,     3,     3,     3,     5,   116,   116,     3,   117,     3,
     116,   116,   116,   116,   116,   117,     6,   116,     3,     3,
     116,     3,     3,     3,     3,     3,   117,     3,   176,     3,
     176,   117,     3,   176,   117,     3,   176,   117,     3,   176,
       3,   118,     3,     3,     3,     3,     6,   116,   117,   116,
     116,   116,     3,     3,     6,   117,     3,   116,   118,   120,
       3,     3,     3,   116,   118,   120,     3,     3,     5,     3,
       3,     3,     3,     3,     3,     3,     3,   116,     3,     3,
     118,   116,   116,   118,     3,   117,   117,     3,   117,     3,
     117,     3,   117,    10,    11,    12,    14,    15,   179,   118,
     118,   116,     3,     3,   176,   176,     3,   176,   117,     3,
     119,   118,     3,   116,   118,   118,   118,     3,   116,   118,
     118,   118,   119,   118,     3,   182,     3,     3,    14,   179,
     118,     3,     3,   118,     3,   118,     3,   118,     3,   116,
     116,    14,   179,    14,   179,     3,   118,   117,   117,   117,
       3,   116,   118,   120,     3,   116,   118,     3,   176,   116,
     116,   116,   118,     3,   116,   116,     3,   116,   116,   116,
     116,    10,   183,   118,   118,   183,   118,   183,   118,   183,
     118,     3,     3,   116,   116,   116,   116,   182,     3,     3,
       3,   118,     3,   118,   118,     3,   176,   116,   117,     3,
       3,     3,   116,     3,     3,     3,     3,     3,     3,    10,
     183,   183,    10,    10,     3,     3,     3,     3,   116,   118,
     118,   118,   118,   117,     3,     3,     3,     3,     3,   118,
     118
};

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
     172,   171,   171,   174,   173,   175,   173,   176,   176,   178,
     177,   179,   179,   179,   179,   179,   180,   180,   180,   180,
     181,   181,   181,   182,   182,   183,   183
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
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
       0,     5,     1,     0,     4,     0,     8,     0,     1,     0,
       2,     1,     1,     0,     1,     1,     0,     1,     1,     1,
       0,     1,     1,     0,     1,     0,     1
};


#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)
#define YYEMPTY         (-2)
#define YYEOF           0

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                  \
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

/* Error token number */
#define YYTERROR        1
#define YYERRCODE       256



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

/* This macro is provided for backward compatibility. */
#ifndef YY_LOCATION_PRINT
# define YY_LOCATION_PRINT(File, Loc) ((void) 0)
#endif


# define YY_SYMBOL_PRINT(Title, Type, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Type, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*----------------------------------------.
| Print this symbol's value on YYOUTPUT.  |
`----------------------------------------*/

static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
{
  FILE *yyo = yyoutput;
  YYUSE (yyo);
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
  YYUSE (yytype);
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyoutput, "%s %s (",
             yytype < YYNTOKENS ? "token" : "nterm", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
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
yy_reduce_print (yytype_int16 *yyssp, YYSTYPE *yyvsp, int yyrule)
{
  unsigned long int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       yystos[yyssp[yyi + 1 - yynrhs]],
                       &(yyvsp[(yyi + 1) - (yynrhs)])
                                              );
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
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
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


#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
yystrlen (const char *yystr)
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
static char *
yystpcpy (char *yydest, const char *yysrc)
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

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return 1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYSIZE_T *yymsg_alloc, char **yymsg,
                yytype_int16 *yyssp, int yytoken)
{
  YYSIZE_T yysize0 = yytnamerr (YY_NULL, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULL;
  /* Arguments of yyformat. */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int yycount = 0;

  /* There are many possibilities here to consider:
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yytoken != YYEMPTY)
    {
      int yyn = yypact[*yyssp];
      yyarg[yycount++] = yytname[yytoken];
      if (!yypact_value_is_default (yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for
             this state because they are default actions.  */
          int yyxbegin = yyn < 0 ? -yyn : 0;
          /* Stay within bounds of both yycheck and yytname.  */
          int yychecklim = YYLAST - yyn + 1;
          int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
          int yyx;

          for (yyx = yyxbegin; yyx < yyxend; ++yyx)
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR
                && !yytable_value_is_error (yytable[yyx + yyn]))
              {
                if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    yycount = 1;
                    yysize = yysize0;
                    break;
                  }
                yyarg[yycount++] = yytname[yyx];
                {
                  YYSIZE_T yysize1 = yysize + yytnamerr (YY_NULL, yytname[yyx]);
                  if (! (yysize <= yysize1
                         && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
                    return 2;
                  yysize = yysize1;
                }
              }
        }
    }

  switch (yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        yyformat = S;                       \
      break
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  {
    YYSIZE_T yysize1 = yysize + yystrlen (yyformat);
    if (! (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
      return 2;
    yysize = yysize1;
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yyarg[yyi++]);
          yyformat += 2;
        }
      else
        {
          yyp++;
          yyformat++;
        }
  }
  return 0;
}
#endif /* YYERROR_VERBOSE */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
{
  YYUSE (yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE (yytype);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}




/* The lookahead symbol.  */
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
    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       'yyss': related to states.
       'yyvs': related to semantic values.

       Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yyssp = yyss = yyssa;
  yyvsp = yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */
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
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
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

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = yylex ();
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

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

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
        case 2:
#line 175 "./config/rx-parse.y" /* yacc.c:1661  */
    { as_bad (_("Unknown opcode: %s"), rx_init_start); }
#line 1955 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 3:
#line 180 "./config/rx-parse.y" /* yacc.c:1661  */
    { B1 (0x00); }
#line 1961 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 4:
#line 183 "./config/rx-parse.y" /* yacc.c:1661  */
    { B1 (0x01); }
#line 1967 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 5:
#line 186 "./config/rx-parse.y" /* yacc.c:1661  */
    { B1 (0x02); }
#line 1973 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 6:
#line 189 "./config/rx-parse.y" /* yacc.c:1661  */
    { B1 (0x03); }
#line 1979 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 7:
#line 194 "./config/rx-parse.y" /* yacc.c:1661  */
    { if (rx_disp3op ((yyvsp[0].exp)))
	      { B1 (0x08); rx_disp3 ((yyvsp[0].exp), 5); }
	    else if (rx_intop ((yyvsp[0].exp), 8, 8))
	      { B1 (0x2e); PC1 ((yyvsp[0].exp)); }
	    else if (rx_intop ((yyvsp[0].exp), 16, 16))
	      { B1 (0x38); PC2 ((yyvsp[0].exp)); }
	    else if (rx_intop ((yyvsp[0].exp), 24, 24))
	      { B1 (0x04); PC3 ((yyvsp[0].exp)); }
	    else
	      { rx_relax (RX_RELAX_BRANCH, 0);
		rx_linkrelax_branch ();
		/* We'll convert this to a longer one later if needed.  */
		B1 (0x08); rx_disp3 ((yyvsp[0].exp), 5); } }
#line 1997 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 8:
#line 209 "./config/rx-parse.y" /* yacc.c:1661  */
    { B1 (0x04); PC3 ((yyvsp[0].exp)); }
#line 2003 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 9:
#line 212 "./config/rx-parse.y" /* yacc.c:1661  */
    { B1 (0x08); rx_disp3 ((yyvsp[0].exp), 5); }
#line 2009 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 10:
#line 217 "./config/rx-parse.y" /* yacc.c:1661  */
    { if (rx_intop ((yyvsp[0].exp), 16, 16))
	      { B1 (0x39); PC2 ((yyvsp[0].exp)); }
	    else if (rx_intop ((yyvsp[0].exp), 24, 24))
	      { B1 (0x05); PC3 ((yyvsp[0].exp)); }
	    else
	      { rx_relax (RX_RELAX_BRANCH, 0);
		rx_linkrelax_branch ();
		B1 (0x39); PC2 ((yyvsp[0].exp)); } }
#line 2022 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 11:
#line 226 "./config/rx-parse.y" /* yacc.c:1661  */
    { B1 (0x05), PC3 ((yyvsp[0].exp)); }
#line 2028 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 12:
#line 231 "./config/rx-parse.y" /* yacc.c:1661  */
    { if ((yyvsp[-2].regno) == COND_EQ || (yyvsp[-2].regno) == COND_NE)
	      { B1 ((yyvsp[-2].regno) == COND_EQ ? 0x10 : 0x18); rx_disp3 ((yyvsp[0].exp), 5); }
	    else
	      as_bad (_("Only BEQ and BNE may have .S")); }
#line 2037 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 13:
#line 239 "./config/rx-parse.y" /* yacc.c:1661  */
    { B1 (0x20); F ((yyvsp[-2].regno), 4, 4); PC1 ((yyvsp[0].exp)); }
#line 2043 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 14:
#line 242 "./config/rx-parse.y" /* yacc.c:1661  */
    { B1 (0x2e), PC1 ((yyvsp[0].exp)); }
#line 2049 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 15:
#line 247 "./config/rx-parse.y" /* yacc.c:1661  */
    { B1 (0x38), PC2 ((yyvsp[0].exp)); }
#line 2055 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 16:
#line 249 "./config/rx-parse.y" /* yacc.c:1661  */
    { B1 (0x39), PC2 ((yyvsp[0].exp)); }
#line 2061 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 17:
#line 251 "./config/rx-parse.y" /* yacc.c:1661  */
    { if ((yyvsp[-2].regno) == COND_EQ || (yyvsp[-2].regno) == COND_NE)
	      { B1 ((yyvsp[-2].regno) == COND_EQ ? 0x3a : 0x3b); PC2 ((yyvsp[0].exp)); }
	    else
	      as_bad (_("Only BEQ and BNE may have .W")); }
#line 2070 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 18:
#line 256 "./config/rx-parse.y" /* yacc.c:1661  */
    { if ((yyvsp[-1].regno) == COND_EQ || (yyvsp[-1].regno) == COND_NE)
	      {
		rx_relax (RX_RELAX_BRANCH, 0);
		rx_linkrelax_branch ();
		B1 ((yyvsp[-1].regno) == COND_EQ ? 0x10 : 0x18); rx_disp3 ((yyvsp[0].exp), 5);
	      }
	    else
	      {
		rx_relax (RX_RELAX_BRANCH, 0);
		/* This is because we might turn it into a
		   jump-over-jump long branch.  */
		rx_linkrelax_branch ();
	        B1 (0x20); F ((yyvsp[-1].regno), 4, 4); PC1 ((yyvsp[0].exp));
	      } }
#line 2089 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 19:
#line 275 "./config/rx-parse.y" /* yacc.c:1661  */
    { if ((yyvsp[-1].regno) <= 7 && rx_uintop ((yyvsp[-5].exp), 8) && rx_disp5op0 (&(yyvsp[-3].exp), BSIZE))
	      { B2 (0x3c, 0); rx_field5s2 ((yyvsp[-3].exp)); F ((yyvsp[-1].regno), 9, 3); O1 ((yyvsp[-5].exp)); }
	    else
	      { B2 (0xf8, 0x04); F ((yyvsp[-1].regno), 8, 4); DSP ((yyvsp[-3].exp), 6, BSIZE); O1 ((yyvsp[-5].exp));
	      if ((yyvsp[-5].exp).X_op != O_constant && (yyvsp[-5].exp).X_op != O_big) rx_linkrelax_imm (12); } }
#line 2099 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 20:
#line 282 "./config/rx-parse.y" /* yacc.c:1661  */
    { if ((yyvsp[-1].regno) <= 7 && rx_uintop ((yyvsp[-5].exp), 8) && rx_disp5op0 (&(yyvsp[-3].exp), WSIZE))
	      { B2 (0x3d, 0); rx_field5s2 ((yyvsp[-3].exp)); F ((yyvsp[-1].regno), 9, 3); O1 ((yyvsp[-5].exp)); }
	    else
	      { B2 (0xf8, 0x01); F ((yyvsp[-1].regno), 8, 4); DSP ((yyvsp[-3].exp), 6, WSIZE); IMMW ((yyvsp[-5].exp), 12); } }
#line 2108 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 21:
#line 288 "./config/rx-parse.y" /* yacc.c:1661  */
    { if ((yyvsp[-1].regno) <= 7 && rx_uintop ((yyvsp[-5].exp), 8) && rx_disp5op0 (&(yyvsp[-3].exp), LSIZE))
	      { B2 (0x3e, 0); rx_field5s2 ((yyvsp[-3].exp)); F ((yyvsp[-1].regno), 9, 3); O1 ((yyvsp[-5].exp)); }
	    else
	      { B2 (0xf8, 0x02); F ((yyvsp[-1].regno), 8, 4); DSP ((yyvsp[-3].exp), 6, LSIZE); IMM ((yyvsp[-5].exp), 12); } }
#line 2117 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 22:
#line 296 "./config/rx-parse.y" /* yacc.c:1661  */
    { B2 (0x3f, 0); F ((yyvsp[-2].regno), 8, 4); F ((yyvsp[0].regno), 12, 4); rtsd_immediate ((yyvsp[-4].exp));
	    if ((yyvsp[-2].regno) == 0)
	      rx_error (_("RTSD cannot pop R0"));
	    if ((yyvsp[-2].regno) > (yyvsp[0].regno))
	      rx_error (_("RTSD first reg must be <= second reg")); }
#line 2127 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 23:
#line 305 "./config/rx-parse.y" /* yacc.c:1661  */
    { B2 (0x47, 0); F ((yyvsp[-2].regno), 8, 4); F ((yyvsp[0].regno), 12, 4); }
#line 2133 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 24:
#line 310 "./config/rx-parse.y" /* yacc.c:1661  */
    { B2 (0x44, 0); F ((yyvsp[-4].regno), 8, 4); F ((yyvsp[0].regno), 12, 4); DSP ((yyvsp[-6].exp), 6, BSIZE); }
#line 2139 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 25:
#line 313 "./config/rx-parse.y" /* yacc.c:1661  */
    { B3 (MEMEX, 0x04, 0); F ((yyvsp[-2].regno), 8, 2);  F ((yyvsp[-4].regno), 16, 4); F ((yyvsp[0].regno), 20, 4); DSP ((yyvsp[-6].exp), 14, sizemap[(yyvsp[-2].regno)]); }
#line 2145 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 26:
#line 318 "./config/rx-parse.y" /* yacc.c:1661  */
    { B2 (0x5b, 0x00); F ((yyvsp[-3].regno), 5, 1); F ((yyvsp[-2].regno), 8, 4); F ((yyvsp[0].regno), 12, 4); }
#line 2151 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 27:
#line 323 "./config/rx-parse.y" /* yacc.c:1661  */
    { B2 (0x58, 0x00); F ((yyvsp[-5].regno), 5, 1); F ((yyvsp[-3].regno), 8, 4); F ((yyvsp[0].regno), 12, 4); }
#line 2157 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 28:
#line 326 "./config/rx-parse.y" /* yacc.c:1661  */
    { if ((yyvsp[-3].regno) <= 7 && (yyvsp[0].regno) <= 7 && rx_disp5op (&(yyvsp[-5].exp), (yyvsp[-6].regno)))
	      { B2 (0xb0, 0); F ((yyvsp[-6].regno), 4, 1); F ((yyvsp[-3].regno), 9, 3); F ((yyvsp[0].regno), 13, 3); rx_field5s ((yyvsp[-5].exp)); }
	    else
	      { B2 (0x58, 0x00); F ((yyvsp[-6].regno), 5, 1); F ((yyvsp[-3].regno), 8, 4); F ((yyvsp[0].regno), 12, 4); DSP ((yyvsp[-5].exp), 6, (yyvsp[-6].regno)); } }
#line 2166 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 29:
#line 334 "./config/rx-parse.y" /* yacc.c:1661  */
    { if (rx_uintop ((yyvsp[-2].exp), 4))
	      { B2 (0x60, 0); FE ((yyvsp[-2].exp), 8, 4); F ((yyvsp[0].regno), 12, 4); }
	    else
	      /* This is really an add, but we negate the immediate.  */
	      { B2 (0x70, 0); F ((yyvsp[0].regno), 8, 4); F ((yyvsp[0].regno), 12, 4); NIMM ((yyvsp[-2].exp), 6); } }
#line 2176 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 30:
#line 341 "./config/rx-parse.y" /* yacc.c:1661  */
    { if (rx_uintop ((yyvsp[-2].exp), 4))
	      { B2 (0x61, 0); FE ((yyvsp[-2].exp), 8, 4); F ((yyvsp[0].regno), 12, 4); }
	    else if (rx_uintop ((yyvsp[-2].exp), 8))
	      { B2 (0x75, 0x50); F ((yyvsp[0].regno), 12, 4); UO1 ((yyvsp[-2].exp)); }
	    else
	      { B2 (0x74, 0x00); F ((yyvsp[0].regno), 12, 4); IMM ((yyvsp[-2].exp), 6); } }
#line 2187 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 31:
#line 349 "./config/rx-parse.y" /* yacc.c:1661  */
    { if (rx_uintop ((yyvsp[-2].exp), 4))
	      { B2 (0x62, 0); FE ((yyvsp[-2].exp), 8, 4); F ((yyvsp[0].regno), 12, 4); }
	    else
	      { B2 (0x70, 0); F ((yyvsp[0].regno), 8, 4); F ((yyvsp[0].regno), 12, 4); IMM ((yyvsp[-2].exp), 6); } }
#line 2196 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 32:
#line 355 "./config/rx-parse.y" /* yacc.c:1661  */
    { if (rx_uintop ((yyvsp[-2].exp), 4))
	      { B2 (0x63, 0); FE ((yyvsp[-2].exp), 8, 4); F ((yyvsp[0].regno), 12, 4); }
	    else
	      { B2 (0x74, 0x10); F ((yyvsp[0].regno), 12, 4); IMM ((yyvsp[-2].exp), 6); } }
#line 2205 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 33:
#line 361 "./config/rx-parse.y" /* yacc.c:1661  */
    { if (rx_uintop ((yyvsp[-2].exp), 4))
	      { B2 (0x64, 0); FE ((yyvsp[-2].exp), 8, 4); F ((yyvsp[0].regno), 12, 4); }
	    else
	      { B2 (0x74, 0x20); F ((yyvsp[0].regno), 12, 4); IMM ((yyvsp[-2].exp), 6); } }
#line 2214 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 34:
#line 367 "./config/rx-parse.y" /* yacc.c:1661  */
    { if (rx_uintop ((yyvsp[-2].exp), 4))
	      { B2 (0x65, 0); FE ((yyvsp[-2].exp), 8, 4); F ((yyvsp[0].regno), 12, 4); }
	    else
	      { B2 (0x74, 0x30); F ((yyvsp[0].regno), 12, 4); IMM ((yyvsp[-2].exp), 6); } }
#line 2223 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 35:
#line 373 "./config/rx-parse.y" /* yacc.c:1661  */
    { if (rx_uintop ((yyvsp[-2].exp), 4))
	      { B2 (0x66, 0); FE ((yyvsp[-2].exp), 8, 4); F ((yyvsp[0].regno), 12, 4); }
	    else if (rx_uintop ((yyvsp[-2].exp), 8))
	      { B2 (0x75, 0x40); F ((yyvsp[0].regno), 12, 4); UO1 ((yyvsp[-2].exp)); }
	    else
	      { B2 (0xfb, 0x02); F ((yyvsp[0].regno), 8, 4); IMM ((yyvsp[-2].exp), 12); } }
#line 2234 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 36:
#line 381 "./config/rx-parse.y" /* yacc.c:1661  */
    { if (rx_uintop ((yyvsp[-2].exp), 4))
	      { B2 (0x66, 0); FE ((yyvsp[-2].exp), 8, 4); F ((yyvsp[0].regno), 12, 4); }
	    else if (rx_uintop ((yyvsp[-2].exp), 8))
	      { B2 (0x75, 0x40); F ((yyvsp[0].regno), 12, 4); UO1 ((yyvsp[-2].exp)); }
	    else
	      { B2 (0xfb, 0x02); F ((yyvsp[0].regno), 8, 4); IMM ((yyvsp[-2].exp), 12); } }
#line 2245 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 37:
#line 391 "./config/rx-parse.y" /* yacc.c:1661  */
    { B1 (0x67); rtsd_immediate ((yyvsp[0].exp)); }
#line 2251 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 38:
#line 395 "./config/rx-parse.y" /* yacc.c:1661  */
    { sub_op = 0; }
#line 2257 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 40:
#line 396 "./config/rx-parse.y" /* yacc.c:1661  */
    { sub_op = 1; }
#line 2263 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 42:
#line 397 "./config/rx-parse.y" /* yacc.c:1661  */
    { sub_op = 2; }
#line 2269 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 44:
#line 402 "./config/rx-parse.y" /* yacc.c:1661  */
    {
	    if ((yyvsp[-2].regno) == (yyvsp[0].regno))
	      { B2 (0x7e, 0x80); F (LSIZE, 10, 2); F ((yyvsp[-2].regno), 12, 4); }
	    else
	     { B2 (0x6e, 0); F ((yyvsp[-2].regno), 8, 4); F ((yyvsp[0].regno), 12, 4); }
	    if ((yyvsp[-2].regno) == 0)
	      rx_error (_("PUSHM cannot push R0"));
	    if ((yyvsp[-2].regno) > (yyvsp[0].regno))
	      rx_error (_("PUSHM first reg must be <= second reg")); }
#line 2283 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 45:
#line 415 "./config/rx-parse.y" /* yacc.c:1661  */
    {
	    if ((yyvsp[-2].regno) == (yyvsp[0].regno))
	      { B2 (0x7e, 0xb0); F ((yyvsp[-2].regno), 12, 4); }
	    else
	      { B2 (0x6f, 0); F ((yyvsp[-2].regno), 8, 4); F ((yyvsp[0].regno), 12, 4); }
	    if ((yyvsp[-2].regno) == 0)
	      rx_error (_("POPM cannot pop R0"));
	    if ((yyvsp[-2].regno) > (yyvsp[0].regno))
	      rx_error (_("POPM first reg must be <= second reg")); }
#line 2297 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 46:
#line 428 "./config/rx-parse.y" /* yacc.c:1661  */
    { B2 (0x70, 0x00); F ((yyvsp[-2].regno), 8, 4); F ((yyvsp[0].regno), 12, 4); IMM ((yyvsp[-4].exp), 6); }
#line 2303 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 47:
#line 433 "./config/rx-parse.y" /* yacc.c:1661  */
    { B2(0x75, 0x60), UO1 ((yyvsp[0].exp)); }
#line 2309 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 48:
#line 438 "./config/rx-parse.y" /* yacc.c:1661  */
    { B2 (0x78, 0); FE ((yyvsp[-2].exp), 7, 5); F ((yyvsp[0].regno), 12, 4); }
#line 2315 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 49:
#line 440 "./config/rx-parse.y" /* yacc.c:1661  */
    { B2 (0x7a, 0); FE ((yyvsp[-2].exp), 7, 5); F ((yyvsp[0].regno), 12, 4); }
#line 2321 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 50:
#line 445 "./config/rx-parse.y" /* yacc.c:1661  */
    { B2 (0x7c, 0x00); FE ((yyvsp[-2].exp), 7, 5); F ((yyvsp[0].regno), 12, 4); }
#line 2327 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 51:
#line 450 "./config/rx-parse.y" /* yacc.c:1661  */
    { B2 (0x7e, 0x30); F ((yyvsp[0].regno), 12, 4); }
#line 2333 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 52:
#line 452 "./config/rx-parse.y" /* yacc.c:1661  */
    { B2 (0x7e, 0x40); F ((yyvsp[0].regno), 12, 4); }
#line 2339 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 53:
#line 454 "./config/rx-parse.y" /* yacc.c:1661  */
    { B2 (0x7e, 0x50); F ((yyvsp[0].regno), 12, 4); }
#line 2345 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 54:
#line 459 "./config/rx-parse.y" /* yacc.c:1661  */
    { B2 (0x7e, 0x80); F ((yyvsp[-1].regno), 10, 2); F ((yyvsp[0].regno), 12, 4); }
#line 2351 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 55:
#line 464 "./config/rx-parse.y" /* yacc.c:1661  */
    { B2 (0x7e, 0xb0); F ((yyvsp[0].regno), 12, 4); }
#line 2357 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 56:
#line 469 "./config/rx-parse.y" /* yacc.c:1661  */
    { if ((yyvsp[0].regno) < 16)
	      { B2 (0x7e, 0xc0); F ((yyvsp[0].regno), 12, 4); }
	    else
	      as_bad (_("PUSHC can only push the first 16 control registers")); }
#line 2366 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 57:
#line 477 "./config/rx-parse.y" /* yacc.c:1661  */
    { if ((yyvsp[0].regno) < 16)
	      { B2 (0x7e, 0xe0); F ((yyvsp[0].regno), 12, 4); }
	    else
	      as_bad (_("POPC can only pop the first 16 control registers")); }
#line 2375 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 58:
#line 485 "./config/rx-parse.y" /* yacc.c:1661  */
    { B2 (0x7f, 0xa0); F ((yyvsp[0].regno), 12, 4); }
#line 2381 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 59:
#line 487 "./config/rx-parse.y" /* yacc.c:1661  */
    { B2 (0x7f, 0xb0); F ((yyvsp[0].regno), 12, 4); }
#line 2387 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 60:
#line 492 "./config/rx-parse.y" /* yacc.c:1661  */
    { B2 (0x7f, 0x00); F ((yyvsp[0].regno), 12, 4); }
#line 2393 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 61:
#line 494 "./config/rx-parse.y" /* yacc.c:1661  */
    { B2 (0x7f, 0x10); F ((yyvsp[0].regno), 12, 4); }
#line 2399 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 62:
#line 496 "./config/rx-parse.y" /* yacc.c:1661  */
    { B2 (0x7f, 0x40); F ((yyvsp[0].regno), 12, 4); }
#line 2405 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 63:
#line 498 "./config/rx-parse.y" /* yacc.c:1661  */
    { B2 (0x7f, 0x50); F ((yyvsp[0].regno), 12, 4); }
#line 2411 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 64:
#line 503 "./config/rx-parse.y" /* yacc.c:1661  */
    { B2 (0x7f, 0x83); rx_note_string_insn_use (); }
#line 2417 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 65:
#line 505 "./config/rx-parse.y" /* yacc.c:1661  */
    { B2 (0x7f, 0x87); rx_note_string_insn_use (); }
#line 2423 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 66:
#line 507 "./config/rx-parse.y" /* yacc.c:1661  */
    { B2 (0x7f, 0x8b); rx_note_string_insn_use (); }
#line 2429 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 67:
#line 509 "./config/rx-parse.y" /* yacc.c:1661  */
    { B2 (0x7f, 0x8f); rx_note_string_insn_use (); }
#line 2435 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 68:
#line 514 "./config/rx-parse.y" /* yacc.c:1661  */
    { B2 (0x7f, 0x80); F ((yyvsp[0].regno), 14, 2); rx_note_string_insn_use (); }
#line 2441 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 69:
#line 516 "./config/rx-parse.y" /* yacc.c:1661  */
    { B2 (0x7f, 0x84); F ((yyvsp[0].regno), 14, 2); rx_note_string_insn_use (); }
#line 2447 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 70:
#line 518 "./config/rx-parse.y" /* yacc.c:1661  */
    { B2 (0x7f, 0x88); F ((yyvsp[0].regno), 14, 2); }
#line 2453 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 71:
#line 523 "./config/rx-parse.y" /* yacc.c:1661  */
    { B2 (0x7f, 0x8c); F ((yyvsp[0].regno), 14, 2); rx_note_string_insn_use (); }
#line 2459 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 72:
#line 528 "./config/rx-parse.y" /* yacc.c:1661  */
    { B2 (0x7f, 0x94); }
#line 2465 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 73:
#line 530 "./config/rx-parse.y" /* yacc.c:1661  */
    { B2 (0x7f, 0x95); }
#line 2471 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 74:
#line 532 "./config/rx-parse.y" /* yacc.c:1661  */
    { B2 (0x7f, 0x96); }
#line 2477 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 75:
#line 534 "./config/rx-parse.y" /* yacc.c:1661  */
    { B2 (0x7f, 0x93); }
#line 2483 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 76:
#line 539 "./config/rx-parse.y" /* yacc.c:1661  */
    { B3 (0x75, 0x70, 0x00); FE ((yyvsp[0].exp), 20, 4); }
#line 2489 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 77:
#line 545 "./config/rx-parse.y" /* yacc.c:1661  */
    { if ((yyvsp[-5].regno) <= 7 && (yyvsp[-1].regno) <= 7 && rx_disp5op (&(yyvsp[-3].exp), (yyvsp[-6].regno)))
	      { B2 (0x80, 0); F ((yyvsp[-6].regno), 2, 2); F ((yyvsp[-1].regno), 9, 3); F ((yyvsp[-5].regno), 13, 3); rx_field5s ((yyvsp[-3].exp)); }
	    else
	      { B2 (0xc3, 0x00); F ((yyvsp[-6].regno), 2, 2); F ((yyvsp[-1].regno), 8, 4); F ((yyvsp[-5].regno), 12, 4); DSP ((yyvsp[-3].exp), 4, (yyvsp[-6].regno)); }}
#line 2498 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 78:
#line 553 "./config/rx-parse.y" /* yacc.c:1661  */
    { if ((yyvsp[-3].regno) <= 7 && (yyvsp[0].regno) <= 7 && rx_disp5op (&(yyvsp[-5].exp), (yyvsp[-6].regno)))
	      { B2 (0x88, 0); F ((yyvsp[-6].regno), 2, 2); F ((yyvsp[-3].regno), 9, 3); F ((yyvsp[0].regno), 13, 3); rx_field5s ((yyvsp[-5].exp)); }
	    else
	      { B2 (0xcc, 0x00); F ((yyvsp[-6].regno), 2, 2); F ((yyvsp[-3].regno), 8, 4); F ((yyvsp[0].regno), 12, 4); DSP ((yyvsp[-5].exp), 6, (yyvsp[-6].regno)); } }
#line 2507 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 79:
#line 567 "./config/rx-parse.y" /* yacc.c:1661  */
    { B2 (0xc3, 0x00); F ((yyvsp[-5].regno), 2, 2); F ((yyvsp[-1].regno), 8, 4); F ((yyvsp[-4].regno), 12, 4); }
#line 2513 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 80:
#line 572 "./config/rx-parse.y" /* yacc.c:1661  */
    { B2 (0xc0, 0); F ((yyvsp[-8].regno), 2, 2); F ((yyvsp[-6].regno), 8, 4); F ((yyvsp[-1].regno), 12, 4); DSP ((yyvsp[-3].exp), 4, (yyvsp[-8].regno)); }
#line 2519 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 81:
#line 577 "./config/rx-parse.y" /* yacc.c:1661  */
    { B2 (0xc0, 0x00); F ((yyvsp[-9].regno), 2, 2); F ((yyvsp[-6].regno), 8, 4); F ((yyvsp[-1].regno), 12, 4); DSP ((yyvsp[-8].exp), 6, (yyvsp[-9].regno)); DSP ((yyvsp[-3].exp), 4, (yyvsp[-9].regno)); }
#line 2525 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 82:
#line 582 "./config/rx-parse.y" /* yacc.c:1661  */
    { B2 (0xcf, 0x00); F ((yyvsp[-3].regno), 2, 2); F ((yyvsp[-2].regno), 8, 4); F ((yyvsp[0].regno), 12, 4); }
#line 2531 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 83:
#line 587 "./config/rx-parse.y" /* yacc.c:1661  */
    { B2 (0xcc, 0x00); F ((yyvsp[-5].regno), 2, 2); F ((yyvsp[-3].regno), 8, 4); F ((yyvsp[0].regno), 12, 4); }
#line 2537 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 84:
#line 592 "./config/rx-parse.y" /* yacc.c:1661  */
    { B2 (0xf0, 0x00); F ((yyvsp[-2].regno), 8, 4); FE ((yyvsp[-6].exp), 13, 3); DSP ((yyvsp[-4].exp), 6, BSIZE); }
#line 2543 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 85:
#line 594 "./config/rx-parse.y" /* yacc.c:1661  */
    { B2 (0xf0, 0x08); F ((yyvsp[-2].regno), 8, 4); FE ((yyvsp[-6].exp), 13, 3); DSP ((yyvsp[-4].exp), 6, BSIZE); }
#line 2549 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 86:
#line 596 "./config/rx-parse.y" /* yacc.c:1661  */
    { B2 (0xf4, 0x00); F ((yyvsp[-2].regno), 8, 4); FE ((yyvsp[-6].exp), 13, 3); DSP ((yyvsp[-4].exp), 6, BSIZE); }
#line 2555 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 87:
#line 601 "./config/rx-parse.y" /* yacc.c:1661  */
    { B2 (0xf4, 0x08); F ((yyvsp[-4].regno), 14, 2); F ((yyvsp[-1].regno), 8, 4); DSP ((yyvsp[-3].exp), 6, (yyvsp[-4].regno)); }
#line 2561 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 88:
#line 605 "./config/rx-parse.y" /* yacc.c:1661  */
    { sub_op = 0; }
#line 2567 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 90:
#line 606 "./config/rx-parse.y" /* yacc.c:1661  */
    { sub_op = 1; sub_op2 = 1; }
#line 2573 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 92:
#line 607 "./config/rx-parse.y" /* yacc.c:1661  */
    { sub_op = 2; }
#line 2579 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 94:
#line 608 "./config/rx-parse.y" /* yacc.c:1661  */
    { sub_op = 3; sub_op2 = 2; }
#line 2585 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 96:
#line 609 "./config/rx-parse.y" /* yacc.c:1661  */
    { sub_op = 4; }
#line 2591 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 98:
#line 610 "./config/rx-parse.y" /* yacc.c:1661  */
    { sub_op = 5; }
#line 2597 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 100:
#line 611 "./config/rx-parse.y" /* yacc.c:1661  */
    { sub_op = 6; }
#line 2603 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 102:
#line 612 "./config/rx-parse.y" /* yacc.c:1661  */
    { sub_op = 7; }
#line 2609 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 104:
#line 613 "./config/rx-parse.y" /* yacc.c:1661  */
    { sub_op = 8; }
#line 2615 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 106:
#line 614 "./config/rx-parse.y" /* yacc.c:1661  */
    { sub_op = 9; }
#line 2621 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 108:
#line 615 "./config/rx-parse.y" /* yacc.c:1661  */
    { sub_op = 12; }
#line 2627 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 110:
#line 616 "./config/rx-parse.y" /* yacc.c:1661  */
    { sub_op = 13; }
#line 2633 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 112:
#line 617 "./config/rx-parse.y" /* yacc.c:1661  */
    { sub_op = 14; sub_op2 = 0; }
#line 2639 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 114:
#line 618 "./config/rx-parse.y" /* yacc.c:1661  */
    { sub_op = 14; }
#line 2645 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 116:
#line 619 "./config/rx-parse.y" /* yacc.c:1661  */
    { sub_op = 15; }
#line 2651 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 118:
#line 623 "./config/rx-parse.y" /* yacc.c:1661  */
    { sub_op = 6; }
#line 2657 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 120:
#line 624 "./config/rx-parse.y" /* yacc.c:1661  */
    { sub_op = 7; }
#line 2663 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 122:
#line 625 "./config/rx-parse.y" /* yacc.c:1661  */
    { sub_op = 16; }
#line 2669 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 124:
#line 626 "./config/rx-parse.y" /* yacc.c:1661  */
    { sub_op = 17; }
#line 2675 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 126:
#line 631 "./config/rx-parse.y" /* yacc.c:1661  */
    { id24 (1, 0x63, 0x00); F ((yyvsp[0].regno), 16, 4); F ((yyvsp[-2].regno), 20, 4); }
#line 2681 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 127:
#line 633 "./config/rx-parse.y" /* yacc.c:1661  */
    { id24 (1, 0x67, 0x00); F ((yyvsp[0].regno), 16, 4); F ((yyvsp[-2].regno), 20, 4); }
#line 2687 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 128:
#line 635 "./config/rx-parse.y" /* yacc.c:1661  */
    { id24 (1, 0x6b, 0x00); F ((yyvsp[0].regno), 16, 4); F ((yyvsp[-2].regno), 20, 4); }
#line 2693 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 129:
#line 637 "./config/rx-parse.y" /* yacc.c:1661  */
    { id24 (1, 0x6f, 0x00); F ((yyvsp[0].regno), 16, 4); F ((yyvsp[-2].regno), 20, 4); }
#line 2699 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 130:
#line 640 "./config/rx-parse.y" /* yacc.c:1661  */
    { id24 (1, 0x60, 0x00); F ((yyvsp[-2].regno), 16, 4); F ((yyvsp[-6].regno), 20, 4); DSP ((yyvsp[-4].exp), 14, BSIZE); }
#line 2705 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 131:
#line 642 "./config/rx-parse.y" /* yacc.c:1661  */
    { id24 (1, 0x64, 0x00); F ((yyvsp[-2].regno), 16, 4); F ((yyvsp[-6].regno), 20, 4); DSP ((yyvsp[-4].exp), 14, BSIZE); }
#line 2711 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 132:
#line 644 "./config/rx-parse.y" /* yacc.c:1661  */
    { id24 (1, 0x68, 0x00); F ((yyvsp[-2].regno), 16, 4); F ((yyvsp[-6].regno), 20, 4); DSP ((yyvsp[-4].exp), 14, BSIZE); }
#line 2717 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 133:
#line 646 "./config/rx-parse.y" /* yacc.c:1661  */
    { id24 (1, 0x6c, 0x00); F ((yyvsp[-2].regno), 16, 4); F ((yyvsp[-6].regno), 20, 4); DSP ((yyvsp[-4].exp), 14, BSIZE); }
#line 2723 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 134:
#line 650 "./config/rx-parse.y" /* yacc.c:1661  */
    { sub_op = 0; }
#line 2729 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 136:
#line 651 "./config/rx-parse.y" /* yacc.c:1661  */
    { sub_op = 1; }
#line 2735 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 138:
#line 652 "./config/rx-parse.y" /* yacc.c:1661  */
    { sub_op = 2; }
#line 2741 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 140:
#line 653 "./config/rx-parse.y" /* yacc.c:1661  */
    { sub_op = 3; }
#line 2747 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 142:
#line 654 "./config/rx-parse.y" /* yacc.c:1661  */
    { sub_op = 4; }
#line 2753 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 144:
#line 655 "./config/rx-parse.y" /* yacc.c:1661  */
    { sub_op = 5; }
#line 2759 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 146:
#line 656 "./config/rx-parse.y" /* yacc.c:1661  */
    { sub_op = 6; }
#line 2765 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 148:
#line 661 "./config/rx-parse.y" /* yacc.c:1661  */
    { id24 (1, 0xdb, 0x00); F ((yyvsp[-2].regno), 20, 4); F ((yyvsp[0].regno), 16, 4); }
#line 2771 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 149:
#line 663 "./config/rx-parse.y" /* yacc.c:1661  */
    { id24 (1, 0xd0, 0x00); F ((yyvsp[-5].regno), 20, 4); F ((yyvsp[-4].regno), 12, 2); F ((yyvsp[-1].regno), 16, 4); DSP ((yyvsp[-3].exp), 14, (yyvsp[-4].regno)); }
#line 2777 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 150:
#line 668 "./config/rx-parse.y" /* yacc.c:1661  */
    { id24 (1, 0xe0, 0x00); F ((yyvsp[-8].regno), 20, 4); FE ((yyvsp[-6].exp), 11, 3);
	      F ((yyvsp[-2].regno), 16, 4); DSP ((yyvsp[-4].exp), 14, BSIZE); }
#line 2784 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 151:
#line 674 "./config/rx-parse.y" /* yacc.c:1661  */
    { id24 (1, 0xe0, 0x0f); FE ((yyvsp[-6].exp), 11, 3); F ((yyvsp[-2].regno), 16, 4);
	      DSP ((yyvsp[-4].exp), 14, BSIZE); }
#line 2791 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 152:
#line 680 "./config/rx-parse.y" /* yacc.c:1661  */
    { id24 (2, 0x00, 0x00); F ((yyvsp[-2].regno), 16, 4); F ((yyvsp[0].regno), 20, 4); }
#line 2797 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 153:
#line 682 "./config/rx-parse.y" /* yacc.c:1661  */
    { id24 (2, 0x01, 0x00); F ((yyvsp[-2].regno), 16, 4); F ((yyvsp[0].regno), 20, 4); }
#line 2803 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 154:
#line 684 "./config/rx-parse.y" /* yacc.c:1661  */
    { id24 (2, 0x04, 0x00); F ((yyvsp[-2].regno), 16, 4); F ((yyvsp[0].regno), 20, 4); }
#line 2809 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 155:
#line 686 "./config/rx-parse.y" /* yacc.c:1661  */
    { id24 (2, 0x05, 0x00); F ((yyvsp[-2].regno), 16, 4); F ((yyvsp[0].regno), 20, 4); }
#line 2815 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 156:
#line 692 "./config/rx-parse.y" /* yacc.c:1661  */
    { id24 (2, 0x17, 0x00); F ((yyvsp[0].regno), 20, 4); }
#line 2821 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 157:
#line 694 "./config/rx-parse.y" /* yacc.c:1661  */
    { id24 (2, 0x17, 0x10); F ((yyvsp[0].regno), 20, 4); }
#line 2827 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 158:
#line 696 "./config/rx-parse.y" /* yacc.c:1661  */
    { id24 (2, 0x1f, 0x00); F ((yyvsp[0].regno), 20, 4); }
#line 2833 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 159:
#line 698 "./config/rx-parse.y" /* yacc.c:1661  */
    { id24 (2, 0x1f, 0x20); F ((yyvsp[0].regno), 20, 4); }
#line 2839 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 160:
#line 700 "./config/rx-parse.y" /* yacc.c:1661  */
    { id24 (2, 0x1f, 0x10); F ((yyvsp[0].regno), 20, 4); }
#line 2845 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 161:
#line 703 "./config/rx-parse.y" /* yacc.c:1661  */
    { id24 (2, 0x18, 0x00);
	    if (rx_uintop ((yyvsp[0].exp), 4) && (yyvsp[0].exp).X_add_number == 1)
	      ;
	    else if (rx_uintop ((yyvsp[0].exp), 4) && (yyvsp[0].exp).X_add_number == 2)
	      F (1, 19, 1);
	    else
	      as_bad (_("RACW expects #1 or #2"));}
#line 2857 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 162:
#line 714 "./config/rx-parse.y" /* yacc.c:1661  */
    { id24 (2, 0x20, 0); F ((yyvsp[-6].regno), 14, 2); F ((yyvsp[-2].regno), 16, 4); F ((yyvsp[-5].regno), 20, 4); }
#line 2863 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 163:
#line 716 "./config/rx-parse.y" /* yacc.c:1661  */
    { id24 (2, 0x24, 0); F ((yyvsp[-6].regno), 14, 2); F ((yyvsp[-1].regno), 16, 4); F ((yyvsp[-5].regno), 20, 4); }
#line 2869 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 164:
#line 721 "./config/rx-parse.y" /* yacc.c:1661  */
    { id24 (2, 0x28, 0); F ((yyvsp[-6].regno), 14, 2); F ((yyvsp[-4].regno), 16, 4); F ((yyvsp[0].regno), 20, 4); }
#line 2875 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 165:
#line 723 "./config/rx-parse.y" /* yacc.c:1661  */
    { id24 (2, 0x2c, 0); F ((yyvsp[-6].regno), 14, 2); F ((yyvsp[-3].regno), 16, 4); F ((yyvsp[0].regno), 20, 4); }
#line 2881 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 166:
#line 728 "./config/rx-parse.y" /* yacc.c:1661  */
    { id24 (2, 0x38, 0); F ((yyvsp[-6].regno), 15, 1); F ((yyvsp[-4].regno), 16, 4); F ((yyvsp[0].regno), 20, 4); }
#line 2887 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 167:
#line 730 "./config/rx-parse.y" /* yacc.c:1661  */
    { id24 (2, 0x3c, 0); F ((yyvsp[-6].regno), 15, 1); F ((yyvsp[-3].regno), 16, 4); F ((yyvsp[0].regno), 20, 4); }
#line 2893 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 168:
#line 734 "./config/rx-parse.y" /* yacc.c:1661  */
    { sub_op = 6; }
#line 2899 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 170:
#line 735 "./config/rx-parse.y" /* yacc.c:1661  */
    { sub_op = 4; }
#line 2905 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 172:
#line 736 "./config/rx-parse.y" /* yacc.c:1661  */
    { sub_op = 5; }
#line 2911 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 174:
#line 737 "./config/rx-parse.y" /* yacc.c:1661  */
    { sub_op = 7; }
#line 2917 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 176:
#line 742 "./config/rx-parse.y" /* yacc.c:1661  */
    { id24 (2, 0x68, 0x00); F ((yyvsp[0].regno) % 16, 20, 4); F ((yyvsp[0].regno) / 16, 15, 1);
	    F ((yyvsp[-2].regno), 16, 4); }
#line 2924 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 177:
#line 748 "./config/rx-parse.y" /* yacc.c:1661  */
    { id24 (2, 0x6a, 0); F ((yyvsp[-2].regno), 15, 5); F ((yyvsp[0].regno), 20, 4); }
#line 2930 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 178:
#line 753 "./config/rx-parse.y" /* yacc.c:1661  */
    { id24 (2, 0x6e, 0); FE ((yyvsp[-2].exp), 15, 5); F ((yyvsp[0].regno), 20, 4); }
#line 2936 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 179:
#line 755 "./config/rx-parse.y" /* yacc.c:1661  */
    { id24 (2, 0x6c, 0); FE ((yyvsp[-2].exp), 15, 5); F ((yyvsp[0].regno), 20, 4); }
#line 2942 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 180:
#line 760 "./config/rx-parse.y" /* yacc.c:1661  */
    { id24 (2, 0x73, 0x00); F ((yyvsp[0].regno), 19, 5); IMM ((yyvsp[-2].exp), 12); }
#line 2948 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 181:
#line 765 "./config/rx-parse.y" /* yacc.c:1661  */
    { id24 (2, 0xe0, 0x00); F ((yyvsp[-4].regno), 16, 4); FE ((yyvsp[-2].exp), 11, 5);
	      F ((yyvsp[0].regno), 20, 4); }
#line 2955 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 182:
#line 771 "./config/rx-parse.y" /* yacc.c:1661  */
    { id24 (2, 0xe0, 0xf0); FE ((yyvsp[-2].exp), 11, 5); F ((yyvsp[0].regno), 20, 4); }
#line 2961 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 183:
#line 776 "./config/rx-parse.y" /* yacc.c:1661  */
    { id24 (3, 0x00, 0); F ((yyvsp[-7].regno), 10, 2); F ((yyvsp[-3].regno), 12, 4); F ((yyvsp[-1].regno), 16, 4); F ((yyvsp[-6].regno), 20, 4); }
#line 2967 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 184:
#line 779 "./config/rx-parse.y" /* yacc.c:1661  */
    { id24 (3, 0x40, 0); F ((yyvsp[-7].regno), 10, 2); F ((yyvsp[-5].regno), 12, 4); F ((yyvsp[-3].regno), 16, 4); F ((yyvsp[0].regno), 20, 4); }
#line 2973 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 185:
#line 782 "./config/rx-parse.y" /* yacc.c:1661  */
    { id24 (3, 0xc0, 0); F ((yyvsp[-7].regno), 10, 2); F ((yyvsp[-5].regno), 12, 4); F ((yyvsp[-3].regno), 16, 4); F ((yyvsp[0].regno), 20, 4); }
#line 2979 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 186:
#line 786 "./config/rx-parse.y" /* yacc.c:1661  */
    { sub_op = 0; }
#line 2985 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 188:
#line 787 "./config/rx-parse.y" /* yacc.c:1661  */
    { sub_op = 2; }
#line 2991 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 190:
#line 788 "./config/rx-parse.y" /* yacc.c:1661  */
    { sub_op = 3; }
#line 2997 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 192:
#line 789 "./config/rx-parse.y" /* yacc.c:1661  */
    { sub_op = 4; }
#line 3003 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 194:
#line 790 "./config/rx-parse.y" /* yacc.c:1661  */
    { sub_op = 5; }
#line 3009 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 196:
#line 796 "./config/rx-parse.y" /* yacc.c:1661  */
    { id24 (2, 0x70, 0x20); F ((yyvsp[0].regno), 20, 4); NBIMM ((yyvsp[-2].exp), 12); }
#line 3015 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 197:
#line 806 "./config/rx-parse.y" /* yacc.c:1661  */
    { B2 (0x43 + (sub_op<<2), 0); F ((yyvsp[-2].regno), 8, 4); F ((yyvsp[0].regno), 12, 4); }
#line 3021 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 198:
#line 808 "./config/rx-parse.y" /* yacc.c:1661  */
    { B2 (0x40 + (sub_op<<2), 0); F ((yyvsp[-4].regno), 8, 4); F ((yyvsp[0].regno), 12, 4); DSP ((yyvsp[-6].exp), 6, BSIZE); }
#line 3027 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 199:
#line 810 "./config/rx-parse.y" /* yacc.c:1661  */
    { B3 (MEMEX, sub_op<<2, 0); F ((yyvsp[-2].regno), 8, 2); F ((yyvsp[-4].regno), 16, 4); F ((yyvsp[0].regno), 20, 4); DSP ((yyvsp[-6].exp), 14, sizemap[(yyvsp[-2].regno)]); }
#line 3033 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 200:
#line 812 "./config/rx-parse.y" /* yacc.c:1661  */
    { id24 (4, sub_op<<4, 0), F ((yyvsp[0].regno), 12, 4), F ((yyvsp[-4].regno), 16, 4), F ((yyvsp[-2].regno), 20, 4); }
#line 3039 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 201:
#line 819 "./config/rx-parse.y" /* yacc.c:1661  */
    { id24 (1, 0x03 + (sub_op<<2), 0x00); F ((yyvsp[-2].regno), 16, 4); F ((yyvsp[0].regno), 20, 4); }
#line 3045 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 202:
#line 821 "./config/rx-parse.y" /* yacc.c:1661  */
    { B4 (MEMEX, 0xa0, 0x00 + sub_op, 0x00);
	  F ((yyvsp[-4].regno), 24, 4); F ((yyvsp[0].regno), 28, 4); DSP ((yyvsp[-6].exp), 14, LSIZE); }
#line 3052 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 203:
#line 829 "./config/rx-parse.y" /* yacc.c:1661  */
    { id24 (1, 0x03 + (sub_op<<2), 0x00); F ((yyvsp[-2].regno), 16, 4); F ((yyvsp[0].regno), 20, 4); }
#line 3058 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 204:
#line 831 "./config/rx-parse.y" /* yacc.c:1661  */
    { id24 (1, 0x00 + (sub_op<<2), 0x00); F ((yyvsp[-4].regno), 16, 4); F ((yyvsp[0].regno), 20, 4); DSP ((yyvsp[-6].exp), 14, BSIZE); }
#line 3064 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 205:
#line 833 "./config/rx-parse.y" /* yacc.c:1661  */
    { B4 (MEMEX, 0x20 + ((yyvsp[-2].regno) << 6), 0x00 + sub_op, 0x00);
	  F ((yyvsp[-4].regno), 24, 4); F ((yyvsp[0].regno), 28, 4); DSP ((yyvsp[-6].exp), 14, sizemap[(yyvsp[-2].regno)]); }
#line 3071 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 206:
#line 839 "./config/rx-parse.y" /* yacc.c:1661  */
    { id24 (2, 0x70, sub_op<<4); F ((yyvsp[0].regno), 20, 4); IMM ((yyvsp[-2].exp), 12); }
#line 3077 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 211:
#line 854 "./config/rx-parse.y" /* yacc.c:1661  */
    { id24 (1, 0x03 + (sub_op<<2), 0x00); F ((yyvsp[-2].regno), 16, 4); F ((yyvsp[0].regno), 20, 4); }
#line 3083 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 212:
#line 856 "./config/rx-parse.y" /* yacc.c:1661  */
    { B2 (0x7e, sub_op2 << 4); F ((yyvsp[0].regno), 12, 4); }
#line 3089 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 213:
#line 862 "./config/rx-parse.y" /* yacc.c:1661  */
    { id24 (1, 0x03 + (sub_op<<2), 0); F ((yyvsp[-2].regno), 16, 4); F ((yyvsp[0].regno), 20, 4); }
#line 3095 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 214:
#line 864 "./config/rx-parse.y" /* yacc.c:1661  */
    { id24 (1, 0x00 + (sub_op<<2), 0); F ((yyvsp[-4].regno), 16, 4); F ((yyvsp[0].regno), 20, 4); DSP ((yyvsp[-6].exp), 14, BSIZE); }
#line 3101 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 215:
#line 866 "./config/rx-parse.y" /* yacc.c:1661  */
    { B4 (MEMEX, 0x20, 0x00 + sub_op, 0); F ((yyvsp[-2].regno), 8, 2); F ((yyvsp[-4].regno), 24, 4); F ((yyvsp[0].regno), 28, 4);
	    DSP ((yyvsp[-6].exp), 14, sizemap[(yyvsp[-2].regno)]); }
#line 3108 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 216:
#line 873 "./config/rx-parse.y" /* yacc.c:1661  */
    { id24 (2, 0x60 + sub_op, 0); F ((yyvsp[-2].regno), 16, 4); F ((yyvsp[0].regno), 20, 4); }
#line 3114 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 217:
#line 877 "./config/rx-parse.y" /* yacc.c:1661  */
    { B2 (0x68 + (sub_op<<1), 0); FE ((yyvsp[-2].exp), 7, 5); F ((yyvsp[0].regno), 12, 4); }
#line 3120 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 218:
#line 879 "./config/rx-parse.y" /* yacc.c:1661  */
    { id24 (2, 0x80 + (sub_op << 5), 0); FE ((yyvsp[-4].exp), 11, 5); F ((yyvsp[-2].regno), 16, 4); F ((yyvsp[0].regno), 20, 4); }
#line 3126 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 220:
#line 885 "./config/rx-parse.y" /* yacc.c:1661  */
    { rx_check_float_support (); }
#line 3132 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 221:
#line 887 "./config/rx-parse.y" /* yacc.c:1661  */
    { id24 (2, 0x72, sub_op << 4); F ((yyvsp[0].regno), 20, 4); O4 ((yyvsp[-2].exp)); }
#line 3138 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 223:
#line 892 "./config/rx-parse.y" /* yacc.c:1661  */
    { rx_check_float_support (); }
#line 3144 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 224:
#line 894 "./config/rx-parse.y" /* yacc.c:1661  */
    { id24 (1, 0x83 + (sub_op << 2), 0); F ((yyvsp[-2].regno), 16, 4); F ((yyvsp[0].regno), 20, 4); }
#line 3150 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 225:
#line 895 "./config/rx-parse.y" /* yacc.c:1661  */
    { rx_check_float_support (); }
#line 3156 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 226:
#line 897 "./config/rx-parse.y" /* yacc.c:1661  */
    { id24 (1, 0x80 + (sub_op << 2), 0); F ((yyvsp[-4].regno), 16, 4); F ((yyvsp[0].regno), 20, 4); DSP ((yyvsp[-6].exp), 14, LSIZE); }
#line 3162 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 227:
#line 902 "./config/rx-parse.y" /* yacc.c:1661  */
    { (yyval.exp) = zero_expr (); }
#line 3168 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 228:
#line 903 "./config/rx-parse.y" /* yacc.c:1661  */
    { (yyval.exp) = (yyvsp[0].exp); }
#line 3174 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 229:
#line 906 "./config/rx-parse.y" /* yacc.c:1661  */
    { need_flag = 1; }
#line 3180 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 230:
#line 906 "./config/rx-parse.y" /* yacc.c:1661  */
    { need_flag = 0; (yyval.regno) = (yyvsp[0].regno); }
#line 3186 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 231:
#line 911 "./config/rx-parse.y" /* yacc.c:1661  */
    { (yyval.regno) = 0; }
#line 3192 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 232:
#line 912 "./config/rx-parse.y" /* yacc.c:1661  */
    { (yyval.regno) = 1; }
#line 3198 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 233:
#line 913 "./config/rx-parse.y" /* yacc.c:1661  */
    { (yyval.regno) = 2; }
#line 3204 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 234:
#line 914 "./config/rx-parse.y" /* yacc.c:1661  */
    { (yyval.regno) = 2; }
#line 3210 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 235:
#line 915 "./config/rx-parse.y" /* yacc.c:1661  */
    { (yyval.regno) = 3; }
#line 3216 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 236:
#line 918 "./config/rx-parse.y" /* yacc.c:1661  */
    { (yyval.regno) = LSIZE; }
#line 3222 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 237:
#line 919 "./config/rx-parse.y" /* yacc.c:1661  */
    { (yyval.regno) = BSIZE; }
#line 3228 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 238:
#line 920 "./config/rx-parse.y" /* yacc.c:1661  */
    { (yyval.regno) = WSIZE; }
#line 3234 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 239:
#line 921 "./config/rx-parse.y" /* yacc.c:1661  */
    { (yyval.regno) = LSIZE; }
#line 3240 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 240:
#line 924 "./config/rx-parse.y" /* yacc.c:1661  */
    { (yyval.regno) = 1; }
#line 3246 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 241:
#line 925 "./config/rx-parse.y" /* yacc.c:1661  */
    { (yyval.regno) = 0; }
#line 3252 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 242:
#line 926 "./config/rx-parse.y" /* yacc.c:1661  */
    { (yyval.regno) = 1; }
#line 3258 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 243:
#line 929 "./config/rx-parse.y" /* yacc.c:1661  */
    {}
#line 3264 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 244:
#line 930 "./config/rx-parse.y" /* yacc.c:1661  */
    {}
#line 3270 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 245:
#line 933 "./config/rx-parse.y" /* yacc.c:1661  */
    {}
#line 3276 "rx-parse.c" /* yacc.c:1661  */
    break;

  case 246:
#line 934 "./config/rx-parse.y" /* yacc.c:1661  */
    {}
#line 3282 "rx-parse.c" /* yacc.c:1661  */
    break;


#line 3286 "rx-parse.c" /* yacc.c:1661  */
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
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (yychar);

  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
# define YYSYNTAX_ERROR yysyntax_error (&yymsg_alloc, &yymsg, \
                                        yyssp, yytoken)
      {
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = YYSYNTAX_ERROR;
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == 1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = (char *) YYSTACK_ALLOC (yymsg_alloc);
            if (!yymsg)
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = 2;
              }
            else
              {
                yysyntax_error_status = YYSYNTAX_ERROR;
                yymsgp = yymsg;
              }
          }
        yyerror (yymsgp);
        if (yysyntax_error_status == 2)
          goto yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
#endif
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

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

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

  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
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

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


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

#if !defined yyoverflow || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
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
  return yyresult;
}
#line 937 "./config/rx-parse.y" /* yacc.c:1906  */

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
      if (0 <= v && v <= 31)
	return 1;
      break;
    case WSIZE:
      if (v & 1)
	return 0;
      if (0 <= v && v <= 63)
	{
	  exp->X_add_number >>= 1;
	  return 1;
	}
      break;
    case LSIZE:
      if (v & 3)
	return 0;
      if (0 <= v && v <= 127)
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

static void
rx_check_float_support (void)
{
  if (rx_cpu == RX100 || rx_cpu == RX200)
    rx_error (_("target CPU type does not support floating point instructions"));
}
