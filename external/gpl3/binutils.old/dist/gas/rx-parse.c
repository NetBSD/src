/* A Bison parser, made by GNU Bison 3.0.4.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015 Free Software Foundation, Inc.

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
#define YYBISON_VERSION "3.0.4"

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

#define id24(a,b2,b3)	   B3 (0xfb + a, b2, b3)

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
static void        rx_check_v2 (void);

static int    need_flag = 0;
static int    rx_in_brackets = 0;
static int    rx_last_token = 0;
static char * rx_init_start;
static char * rx_last_exp_start = 0;
static int    sub_op;
static int    sub_op2;

#define YYDEBUG 1
#define YYERROR_VERBOSE 1


#line 187 "rx-parse.c" /* yacc.c:339  */

# ifndef YY_NULLPTR
#  if defined __cplusplus && 201103L <= __cplusplus
#   define YY_NULLPTR nullptr
#  else
#   define YY_NULLPTR 0
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
    ACC = 261,
    EXPR = 262,
    UNKNOWN_OPCODE = 263,
    IS_OPCODE = 264,
    DOT_S = 265,
    DOT_B = 266,
    DOT_W = 267,
    DOT_L = 268,
    DOT_A = 269,
    DOT_UB = 270,
    DOT_UW = 271,
    ABS = 272,
    ADC = 273,
    ADD = 274,
    AND_ = 275,
    BCLR = 276,
    BCND = 277,
    BMCND = 278,
    BNOT = 279,
    BRA = 280,
    BRK = 281,
    BSET = 282,
    BSR = 283,
    BTST = 284,
    CLRPSW = 285,
    CMP = 286,
    DBT = 287,
    DIV = 288,
    DIVU = 289,
    EDIV = 290,
    EDIVU = 291,
    EMACA = 292,
    EMSBA = 293,
    EMUL = 294,
    EMULA = 295,
    EMULU = 296,
    FADD = 297,
    FCMP = 298,
    FDIV = 299,
    FMUL = 300,
    FREIT = 301,
    FSUB = 302,
    FSQRT = 303,
    FTOI = 304,
    FTOU = 305,
    INT = 306,
    ITOF = 307,
    JMP = 308,
    JSR = 309,
    MACHI = 310,
    MACLH = 311,
    MACLO = 312,
    MAX = 313,
    MIN = 314,
    MOV = 315,
    MOVCO = 316,
    MOVLI = 317,
    MOVU = 318,
    MSBHI = 319,
    MSBLH = 320,
    MSBLO = 321,
    MUL = 322,
    MULHI = 323,
    MULLH = 324,
    MULLO = 325,
    MULU = 326,
    MVFACHI = 327,
    MVFACGU = 328,
    MVFACMI = 329,
    MVFACLO = 330,
    MVFC = 331,
    MVTACGU = 332,
    MVTACHI = 333,
    MVTACLO = 334,
    MVTC = 335,
    MVTIPL = 336,
    NEG = 337,
    NOP = 338,
    NOT = 339,
    OR = 340,
    POP = 341,
    POPC = 342,
    POPM = 343,
    PUSH = 344,
    PUSHA = 345,
    PUSHC = 346,
    PUSHM = 347,
    RACL = 348,
    RACW = 349,
    RDACL = 350,
    RDACW = 351,
    REIT = 352,
    REVL = 353,
    REVW = 354,
    RMPA = 355,
    ROLC = 356,
    RORC = 357,
    ROTL = 358,
    ROTR = 359,
    ROUND = 360,
    RTE = 361,
    RTFI = 362,
    RTS = 363,
    RTSD = 364,
    SAT = 365,
    SATR = 366,
    SBB = 367,
    SCCND = 368,
    SCMPU = 369,
    SETPSW = 370,
    SHAR = 371,
    SHLL = 372,
    SHLR = 373,
    SMOVB = 374,
    SMOVF = 375,
    SMOVU = 376,
    SSTR = 377,
    STNZ = 378,
    STOP = 379,
    STZ = 380,
    SUB = 381,
    SUNTIL = 382,
    SWHILE = 383,
    TST = 384,
    UTOF = 385,
    WAIT = 386,
    XCHG = 387,
    XOR = 388
  };
#endif
/* Tokens.  */
#define REG 258
#define FLAG 259
#define CREG 260
#define ACC 261
#define EXPR 262
#define UNKNOWN_OPCODE 263
#define IS_OPCODE 264
#define DOT_S 265
#define DOT_B 266
#define DOT_W 267
#define DOT_L 268
#define DOT_A 269
#define DOT_UB 270
#define DOT_UW 271
#define ABS 272
#define ADC 273
#define ADD 274
#define AND_ 275
#define BCLR 276
#define BCND 277
#define BMCND 278
#define BNOT 279
#define BRA 280
#define BRK 281
#define BSET 282
#define BSR 283
#define BTST 284
#define CLRPSW 285
#define CMP 286
#define DBT 287
#define DIV 288
#define DIVU 289
#define EDIV 290
#define EDIVU 291
#define EMACA 292
#define EMSBA 293
#define EMUL 294
#define EMULA 295
#define EMULU 296
#define FADD 297
#define FCMP 298
#define FDIV 299
#define FMUL 300
#define FREIT 301
#define FSUB 302
#define FSQRT 303
#define FTOI 304
#define FTOU 305
#define INT 306
#define ITOF 307
#define JMP 308
#define JSR 309
#define MACHI 310
#define MACLH 311
#define MACLO 312
#define MAX 313
#define MIN 314
#define MOV 315
#define MOVCO 316
#define MOVLI 317
#define MOVU 318
#define MSBHI 319
#define MSBLH 320
#define MSBLO 321
#define MUL 322
#define MULHI 323
#define MULLH 324
#define MULLO 325
#define MULU 326
#define MVFACHI 327
#define MVFACGU 328
#define MVFACMI 329
#define MVFACLO 330
#define MVFC 331
#define MVTACGU 332
#define MVTACHI 333
#define MVTACLO 334
#define MVTC 335
#define MVTIPL 336
#define NEG 337
#define NOP 338
#define NOT 339
#define OR 340
#define POP 341
#define POPC 342
#define POPM 343
#define PUSH 344
#define PUSHA 345
#define PUSHC 346
#define PUSHM 347
#define RACL 348
#define RACW 349
#define RDACL 350
#define RDACW 351
#define REIT 352
#define REVL 353
#define REVW 354
#define RMPA 355
#define ROLC 356
#define RORC 357
#define ROTL 358
#define ROTR 359
#define ROUND 360
#define RTE 361
#define RTFI 362
#define RTS 363
#define RTSD 364
#define SAT 365
#define SATR 366
#define SBB 367
#define SCCND 368
#define SCMPU 369
#define SETPSW 370
#define SHAR 371
#define SHLL 372
#define SHLR 373
#define SMOVB 374
#define SMOVF 375
#define SMOVU 376
#define SSTR 377
#define STNZ 378
#define STOP 379
#define STZ 380
#define SUB 381
#define SUNTIL 382
#define SWHILE 383
#define TST 384
#define UTOF 385
#define WAIT 386
#define XCHG 387
#define XOR 388

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 135 "./config/rx-parse.y" /* yacc.c:355  */

  int regno;
  expressionS exp;

#line 498 "rx-parse.c" /* yacc.c:355  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE rx_lval;

int rx_parse (void);

#endif /* !YY_RX_RX_PARSE_H_INCLUDED  */

/* Copy the second part of user declarations.  */

#line 515 "rx-parse.c" /* yacc.c:358  */

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

#ifndef YY_ATTRIBUTE
# if (defined __GNUC__                                               \
      && (2 < __GNUC__ || (__GNUC__ == 2 && 96 <= __GNUC_MINOR__)))  \
     || defined __SUNPRO_C && 0x5110 <= __SUNPRO_C
#  define YY_ATTRIBUTE(Spec) __attribute__(Spec)
# else
#  define YY_ATTRIBUTE(Spec) /* empty */
# endif
#endif

#ifndef YY_ATTRIBUTE_PURE
# define YY_ATTRIBUTE_PURE   YY_ATTRIBUTE ((__pure__))
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# define YY_ATTRIBUTE_UNUSED YY_ATTRIBUTE ((__unused__))
#endif

#if !defined _Noreturn \
     && (!defined __STDC_VERSION__ || __STDC_VERSION__ < 201112)
# if defined _MSC_VER && 1200 <= _MSC_VER
#  define _Noreturn __declspec (noreturn)
# else
#  define _Noreturn YY_ATTRIBUTE ((__noreturn__))
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
#define YYFINAL  255
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   780

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  140
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  76
/* YYNRULES -- Number of rules.  */
#define YYNRULES  294
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  760

/* YYTRANSLATE[YYX] -- Symbol number corresponding to YYX as returned
   by yylex, with out-of-bounds checking.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   388

#define YYTRANSLATE(YYX)                                                \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, without out-of-bounds checking.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,   134,     2,     2,     2,     2,
       2,     2,     2,   139,   135,   138,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   136,     2,   137,     2,     2,     2,     2,     2,     2,
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
     115,   116,   117,   118,   119,   120,   121,   122,   123,   124,
     125,   126,   127,   128,   129,   130,   131,   132,   133
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   178,   178,   183,   186,   189,   192,   197,   212,   215,
     220,   229,   234,   242,   245,   250,   252,   254,   259,   277,
     280,   283,   286,   294,   300,   308,   317,   322,   325,   330,
     335,   338,   346,   353,   361,   367,   373,   379,   385,   393,
     403,   408,   408,   409,   409,   410,   410,   414,   427,   440,
     445,   450,   452,   457,   462,   464,   466,   471,   476,   481,
     491,   501,   503,   508,   510,   512,   514,   519,   521,   523,
     525,   530,   532,   534,   539,   544,   546,   548,   550,   555,
     561,   569,   583,   588,   593,   598,   603,   608,   610,   612,
     617,   622,   622,   623,   623,   624,   624,   625,   625,   626,
     626,   627,   627,   628,   628,   629,   629,   630,   630,   631,
     631,   632,   632,   633,   633,   634,   634,   635,   635,   636,
     636,   640,   640,   641,   641,   642,   642,   643,   643,   644,
     644,   648,   650,   652,   654,   657,   659,   661,   663,   668,
     668,   669,   669,   670,   670,   671,   671,   672,   672,   673,
     673,   674,   674,   675,   675,   676,   676,   683,   685,   690,
     696,   702,   704,   706,   708,   710,   712,   714,   716,   722,
     724,   726,   728,   730,   732,   732,   733,   735,   735,   736,
     738,   738,   739,   747,   758,   760,   765,   767,   772,   774,
     779,   779,   780,   780,   781,   781,   782,   782,   786,   794,
     801,   803,   808,   815,   821,   826,   829,   832,   837,   837,
     838,   838,   839,   839,   840,   840,   841,   841,   846,   851,
     856,   861,   863,   865,   867,   869,   871,   873,   875,   877,
     877,   878,   880,   888,   896,   912,   914,   916,   918,   925,
     927,   935,   937,   939,   945,   950,   951,   955,   956,   960,
     962,   967,   972,   972,   974,   979,   981,   983,   990,   994,
     996,   998,  1002,  1004,  1006,  1008,  1013,  1013,  1016,  1020,
    1020,  1023,  1023,  1029,  1029,  1053,  1054,  1057,  1057,  1062,
    1063,  1064,  1065,  1066,  1069,  1070,  1071,  1072,  1075,  1076,
    1077,  1080,  1081,  1084,  1085
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "REG", "FLAG", "CREG", "ACC", "EXPR",
  "UNKNOWN_OPCODE", "IS_OPCODE", "DOT_S", "DOT_B", "DOT_W", "DOT_L",
  "DOT_A", "DOT_UB", "DOT_UW", "ABS", "ADC", "ADD", "AND_", "BCLR", "BCND",
  "BMCND", "BNOT", "BRA", "BRK", "BSET", "BSR", "BTST", "CLRPSW", "CMP",
  "DBT", "DIV", "DIVU", "EDIV", "EDIVU", "EMACA", "EMSBA", "EMUL", "EMULA",
  "EMULU", "FADD", "FCMP", "FDIV", "FMUL", "FREIT", "FSUB", "FSQRT",
  "FTOI", "FTOU", "INT", "ITOF", "JMP", "JSR", "MACHI", "MACLH", "MACLO",
  "MAX", "MIN", "MOV", "MOVCO", "MOVLI", "MOVU", "MSBHI", "MSBLH", "MSBLO",
  "MUL", "MULHI", "MULLH", "MULLO", "MULU", "MVFACHI", "MVFACGU",
  "MVFACMI", "MVFACLO", "MVFC", "MVTACGU", "MVTACHI", "MVTACLO", "MVTC",
  "MVTIPL", "NEG", "NOP", "NOT", "OR", "POP", "POPC", "POPM", "PUSH",
  "PUSHA", "PUSHC", "PUSHM", "RACL", "RACW", "RDACL", "RDACW", "REIT",
  "REVL", "REVW", "RMPA", "ROLC", "RORC", "ROTL", "ROTR", "ROUND", "RTE",
  "RTFI", "RTS", "RTSD", "SAT", "SATR", "SBB", "SCCND", "SCMPU", "SETPSW",
  "SHAR", "SHLL", "SHLR", "SMOVB", "SMOVF", "SMOVU", "SSTR", "STNZ",
  "STOP", "STZ", "SUB", "SUNTIL", "SWHILE", "TST", "UTOF", "WAIT", "XCHG",
  "XOR", "'#'", "','", "'['", "']'", "'-'", "'+'", "$accept", "statement",
  "$@1", "$@2", "$@3", "$@4", "$@5", "$@6", "$@7", "$@8", "$@9", "$@10",
  "$@11", "$@12", "$@13", "$@14", "$@15", "$@16", "$@17", "$@18", "$@19",
  "$@20", "$@21", "$@22", "$@23", "$@24", "$@25", "$@26", "$@27", "$@28",
  "$@29", "$@30", "$@31", "$@32", "$@33", "$@34", "$@35", "$@36", "$@37",
  "$@38", "$@39", "$@40", "$@41", "$@42", "$@43", "$@44", "$@45",
  "op_subadd", "op_dp20_rm_l", "op_dp20_rm", "op_dp20_i", "op_dp20_rim",
  "op_dp20_rim_l", "op_dp20_rr", "op_dp20_r", "op_dp20_ri", "$@46",
  "op_xchg", "op_shift_rot", "op_shift", "float3_op", "float2_op", "$@47",
  "float2_op_ni", "$@48", "$@49", "mvfa_op", "$@50", "disp", "flag",
  "$@51", "memex", "bwl", "bw", "opt_l", "opt_b", YY_NULLPTR
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
     365,   366,   367,   368,   369,   370,   371,   372,   373,   374,
     375,   376,   377,   378,   379,   380,   381,   382,   383,   384,
     385,   386,   387,   388,    35,    44,    91,    93,    45,    43
};
# endif

#define YYPACT_NINF -600

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-600)))

#define YYTABLE_NINF -270

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     645,  -600,  -600,  -600,  -128,  -112,    14,   114,   -98,    24,
      71,  -600,    28,    60,    29,  -600,    16,  -600,  -600,  -600,
      46,    48,   -75,    68,   -45,  -600,  -600,  -600,  -600,  -600,
    -600,  -600,  -600,   -39,  -600,    98,   112,   132,   151,   167,
    -600,  -600,    34,   169,    54,    57,   194,   203,   205,    81,
     213,   214,   215,   216,  -600,   217,   219,   220,   221,   223,
     224,    30,    95,  -600,  -600,  -600,    96,   225,   226,   230,
     148,   229,   232,   104,   105,   106,   108,  -600,  -600,   148,
     238,   240,   110,   111,  -600,  -600,  -600,  -600,   113,   243,
    -600,   115,   199,  -600,  -600,  -600,  -600,  -600,  -600,  -600,
    -600,   148,  -600,  -600,   116,   148,   148,  -600,  -600,  -600,
    -600,  -600,   248,   249,    21,   244,     5,   246,     5,   120,
     250,  -600,   251,   252,   254,   255,   121,   256,  -600,   257,
     258,   259,  -600,   261,   266,   135,   264,  -600,   267,   268,
     270,   141,   271,  -600,   273,   144,  -600,   274,   146,    22,
      22,   145,   149,   152,     6,   150,   152,     6,    23,    31,
      31,    23,    23,   280,   280,   280,   281,     6,  -600,  -600,
     154,   155,   156,    22,    22,   153,   158,   159,   287,     0,
     160,   293,  -600,  -600,     7,   162,   163,   164,   295,     5,
     165,   168,   170,  -600,  -600,  -600,  -600,  -600,  -600,  -600,
     171,   172,   173,   174,   175,   297,   304,   249,   249,   305,
       5,  -600,  -600,   176,  -600,  -600,  -600,    58,  -600,   177,
     306,   309,   310,   311,   316,   316,  -600,  -600,  -600,   313,
     316,   314,   316,   280,   315,  -600,   317,    89,   320,   318,
    -600,    32,    32,    32,  -600,   152,   152,   319,     5,  -600,
    -600,    22,     6,     6,    22,  -600,   193,  -600,   196,   322,
    -600,  -600,  -600,   191,   198,   201,  -600,   204,   206,  -600,
     124,   207,  -600,  -600,  -600,   208,   179,   209,  -600,  -600,
    -600,  -600,  -600,   181,   210,  -600,  -600,  -600,   184,   211,
    -600,   327,   212,   329,   218,  -600,  -600,  -600,   222,  -600,
     331,   332,  -600,   227,  -600,   228,   334,  -600,  -600,   231,
     341,  -600,   233,  -600,   234,  -600,   335,   318,  -600,  -600,
    -600,  -600,  -600,  -600,  -600,  -600,   336,   346,   347,  -600,
    -600,   344,   345,   348,   235,   236,   237,    -1,   239,   241,
     242,   245,     1,   351,   353,   354,   247,  -600,   356,   357,
     358,  -600,   253,  -600,  -600,  -600,   360,   359,   361,   366,
     369,   260,  -600,  -600,  -600,   265,  -600,   373,  -600,   263,
     376,   275,   276,   277,   278,   279,  -600,  -600,   282,  -600,
     283,  -600,  -600,   284,   285,  -600,  -600,   269,   377,  -600,
    -600,  -600,  -600,  -600,  -600,   380,  -600,   286,  -600,  -600,
    -600,  -600,  -600,   382,   383,   288,   385,   386,   387,   388,
     389,  -600,   272,   186,   189,  -600,   289,   192,  -600,   290,
     195,  -600,   291,   197,  -600,   390,   292,   391,   393,   296,
     298,   399,   400,   299,   403,   300,   406,   408,   301,   294,
     302,   303,   307,   308,   312,   321,   413,     8,   419,    -7,
     421,   425,   323,   429,   436,    40,   437,   324,   325,   326,
     438,   328,   330,   333,   439,  -600,  -600,  -600,  -600,  -600,
     375,   441,  -600,   442,  -600,   443,   444,   445,   446,   450,
     451,   452,   454,   459,   461,   337,   338,  -600,   463,  -600,
    -600,   464,   339,   340,   342,   343,  -600,   466,  -600,   349,
    -600,   350,   467,  -600,   352,   468,  -600,   355,   471,  -600,
     362,  -600,    75,  -600,   363,   472,   473,  -600,   364,   475,
     367,   479,   368,   371,   480,   481,   483,   484,   486,    11,
      33,    13,  -600,  -600,   372,     2,   370,   490,   374,   379,
     381,   384,   491,  -600,   392,   492,   395,   394,   396,   493,
     497,   498,  -600,   504,   505,   506,   397,  -600,  -600,   398,
    -600,  -600,  -600,  -600,  -600,  -600,  -600,   401,  -600,   404,
     494,   514,  -600,  -600,   435,   516,   517,    94,   405,   519,
     520,   407,   521,   409,   522,   410,   523,  -600,  -600,  -600,
     402,  -600,   414,   101,  -600,  -600,   107,  -600,   524,  -600,
     435,   525,  -600,   411,  -600,  -600,  -600,   531,   415,   533,
     416,  -600,   535,   417,   537,    41,   540,   420,   422,   200,
     423,   426,  -600,  -600,   428,   427,   542,   430,   431,  -600,
    -600,  -600,  -600,  -600,  -600,   544,  -600,   551,  -600,   432,
    -600,   433,  -600,  -600,   434,   440,   476,   447,   448,   476,
     449,   476,   453,   476,   455,   553,   554,   456,   458,   460,
     462,  -600,   465,  -600,   435,   457,   557,   469,   567,   470,
     568,   474,   569,  -600,   477,   478,   202,   482,  -600,   485,
     570,   571,   573,   487,  -600,   574,   575,   488,  -600,   576,
     577,   578,   579,  -600,  -600,   572,   476,  -600,   476,  -600,
     585,  -600,   587,  -600,  -600,   584,   586,   596,   598,   599,
     489,  -600,   495,  -600,   496,  -600,   499,  -600,   500,  -600,
    -600,  -600,   502,   601,   602,  -600,  -600,  -600,   605,  -600,
    -600,   606,  -600,  -600,  -600,  -600,  -600,  -600,  -600,  -600,
    -600,  -600,  -600,  -600,  -600,  -600,   607,  -600,  -600,  -600,
    -600,   609,  -600,   503,  -600,  -600,  -600,   507,  -600,  -600
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
       0,     2,    97,    95,   210,   214,     0,     0,     0,     0,
     291,     3,     0,   291,     0,   277,   275,     4,   107,   109,
       0,     0,   121,     0,   123,   143,   141,   147,   145,   139,
     149,   151,   153,     0,   127,     0,     0,     0,     0,     0,
      99,   101,   284,     0,     0,   288,     0,     0,     0,   212,
       0,     0,     0,   174,   229,   177,   180,     0,     0,     0,
       0,     0,     0,    93,     6,   115,   216,     0,     0,     0,
     284,     0,     0,     0,     0,     0,     0,   196,   194,   284,
       0,     0,   190,   192,   155,    76,    75,     5,     0,     0,
      78,    91,   284,    67,   277,    43,    45,    41,    69,    70,
      68,   284,   119,   117,   208,   284,   284,   111,   129,    77,
     125,   113,     0,     0,   275,     0,   275,     0,   275,     0,
       0,    18,     0,     0,     0,     0,     0,     0,     7,     0,
       0,     0,   292,     0,     0,     0,     0,    10,     0,     0,
       0,     0,     0,    62,     0,     0,   276,     0,     0,   275,
     275,     0,     0,     0,   275,     0,     0,   275,   275,   271,
     271,   275,   275,   271,   271,   271,     0,   275,    63,    64,
       0,     0,     0,   275,   275,   285,   286,   287,     0,     0,
       0,     0,   289,   290,     0,     0,     0,     0,     0,   275,
       0,     0,     0,   173,   273,   273,   176,   273,   179,   273,
       0,     0,   169,   171,     0,     0,     0,     0,     0,     0,
     275,    58,    60,     0,   285,   286,   287,   275,    59,     0,
       0,     0,     0,     0,     0,     0,    74,    56,    55,     0,
       0,     0,     0,   271,     0,    54,     0,   275,   287,   275,
      61,     0,     0,     0,    73,   252,   252,     0,   275,    71,
      72,   275,   275,   275,   275,     1,   250,    98,     0,     0,
     247,   248,    96,     0,     0,     0,   211,     0,     0,   215,
     275,     0,    12,    13,    17,     0,   275,     0,     9,    14,
      15,     8,    65,   275,     0,    16,    11,    66,   275,     0,
     278,     0,     0,     0,     0,   245,   246,   108,     0,   110,
       0,     0,   104,     0,   122,     0,     0,   106,   124,     0,
       0,   144,     0,   142,     0,   268,     0,   275,   148,   146,
     140,   150,   152,   154,    50,   128,     0,     0,     0,   100,
     102,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   213,     0,     0,
       0,   175,     0,   230,   178,   181,     0,     0,     0,     0,
       0,     0,    79,    94,   116,     0,   217,     0,    57,     0,
       0,     0,   182,     0,     0,     0,   197,   195,     0,   191,
       0,   193,   156,    40,     0,    92,   157,     0,     0,   261,
      44,    46,    42,   254,   120,     0,   118,     0,   209,   112,
     130,   126,   114,     0,     0,     0,     0,     0,     0,     0,
       0,   132,     0,   275,   275,   134,     0,   275,   131,     0,
     275,   133,     0,   275,    26,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     165,     0,   167,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   161,     0,   163,     0,   199,   231,   170,   172,   198,
       0,     0,    48,     0,    47,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   253,     0,   249,
     239,     0,     0,    34,   235,     0,    36,     0,    52,     0,
     203,     0,     0,   204,     0,     0,    51,     0,     0,    53,
       0,    33,   281,   241,     0,     0,     0,   255,     0,     0,
     263,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    39,    85,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    29,     0,     0,     0,     0,     0,     0,
       0,     0,    35,     0,     0,     0,     0,   202,    37,     0,
     232,   183,   233,   234,   258,   200,   201,     0,   218,     0,
       0,     0,    32,   244,   291,     0,     0,   281,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   279,   280,   282,
       0,   283,     0,   281,   221,   222,   281,   223,     0,   262,
     291,     0,   270,     0,   166,   224,   168,     0,     0,     0,
       0,    38,     0,     0,     0,     0,     0,     0,     0,   275,
       0,     0,   219,   220,     0,     0,     0,     0,     0,   225,
     226,   227,   162,   228,   164,     0,    90,     0,   158,   259,
     251,     0,    49,   238,     0,     0,   293,     0,     0,   293,
       0,   293,     0,   293,     0,     0,     0,     0,     0,     0,
       0,   265,     0,   267,   291,     0,     0,     0,     0,     0,
       0,     0,     0,    82,     0,     0,   275,     0,    86,     0,
       0,     0,     0,     0,    30,     0,     0,     0,    25,     0,
       0,     0,     0,   294,   136,     0,   293,   138,   293,   135,
       0,   137,     0,    27,    28,     0,     0,     0,     0,     0,
       0,    19,     0,    20,     0,    21,     0,    80,     0,   184,
     185,    81,     0,     0,     0,   186,   187,    31,     0,   188,
     189,     0,   260,   240,   236,   237,    88,   159,   160,    87,
      89,   242,   243,   256,   257,   264,     0,    22,    23,    24,
     205,     0,   206,     0,   207,   274,   272,     0,    83,    84
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -600,  -600,  -600,  -600,  -600,  -600,  -600,  -600,  -600,  -600,
    -600,  -600,  -600,  -600,  -600,  -600,  -600,  -600,  -600,  -600,
    -600,  -600,  -600,  -600,  -600,  -600,  -600,  -600,  -600,  -600,
    -600,  -600,  -600,  -600,  -600,  -600,  -600,  -600,  -600,  -600,
    -600,  -600,  -600,  -600,  -600,  -600,  -600,   -81,   412,  -600,
     -93,   -80,  -600,  -131,  -600,   534,  -600,  -119,  -168,  -139,
      52,   501,  -600,  -122,  -600,  -600,   -14,  -600,   -16,   526,
    -600,  -538,   -26,  -600,   -12,  -599
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,   112,   243,   241,   242,   237,   207,   114,   113,   173,
     174,   153,   156,   149,   150,   251,   254,   208,   246,   245,
     154,   157,   253,   167,   252,   162,   159,   158,   161,   160,
     163,   164,   165,   233,   194,   197,   199,   230,   232,   225,
     224,   248,   116,   189,   118,   210,   195,   266,   260,   295,
     296,   297,   262,   257,   487,   394,   395,   304,   389,   390,
     311,   313,   314,   315,   316,   317,   351,   352,   298,   143,
     144,   592,   179,   184,   134,   694
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
     148,   140,   449,   335,   455,   615,   115,   336,   265,   303,
     340,   533,   146,   146,   341,   534,   611,   119,   146,   145,
     146,   261,   117,   146,   258,   294,   309,   126,   146,   146,
     146,   135,   141,   204,  -269,   375,   125,   269,   308,   645,
     146,   321,   322,   323,   217,   175,   176,   177,   325,   151,
     697,   152,   699,   226,   701,   658,   376,   377,   660,  -103,
     302,   368,   379,   307,   381,   146,   239,   137,   182,   183,
     299,   155,   138,   132,   139,   244,   363,   364,   128,   249,
     250,   129,   130,   131,   132,   133,   587,   588,   589,  -105,
     590,   591,   258,   329,   330,   166,   146,   737,   263,   738,
     267,   168,   267,   391,   392,   587,   588,   589,   347,   644,
     591,   382,   587,   588,   589,   169,   657,   591,   587,   588,
     589,   121,   659,   591,   122,   123,   124,   411,   537,   366,
     538,   146,   539,   400,   401,   170,   337,   450,   305,   456,
     616,   305,   312,   342,   535,   312,   312,   607,   120,   612,
     147,   305,   393,   393,   171,   259,   259,   310,   127,   214,
     215,   216,   136,   142,   205,  -266,   388,   398,   178,   609,
     172,   399,   180,   267,   402,   545,   672,   546,   673,   547,
     674,   353,   415,   354,   418,   355,   146,   421,   146,   498,
     181,   146,   500,   146,   267,   503,   146,   185,   506,   146,
     509,   369,   146,   678,   146,   721,   186,   146,   187,   146,
     214,   215,   238,   319,   320,   188,   190,   191,   192,   193,
     196,   263,   198,   387,   201,   200,   202,   203,   211,   206,
     209,   212,   267,   213,   218,   219,   305,   305,   220,   221,
     222,   227,   223,   228,   229,   231,   235,   234,   255,   236,
     247,   264,   256,   268,   412,   270,   276,   271,   272,   273,
     416,   274,   275,   277,   278,   279,   280,   419,   281,   282,
     283,   284,   422,   287,   285,   286,   288,   290,   289,   291,
     300,   292,   293,  -269,   301,   306,   259,   331,   324,   326,
     327,   328,   332,   333,   334,   338,   339,   343,   344,   345,
     348,   439,   346,   349,   361,   350,   356,   357,   358,   359,
     360,   362,   365,   371,   367,   370,   372,   373,   374,   375,
     378,   380,   383,   386,   384,   146,   397,   406,   403,   405,
     424,   404,   426,   407,   429,   430,   408,   433,   438,   440,
     409,   410,   413,   414,   417,   420,   423,   425,   435,   441,
     442,   443,   444,   427,   457,   445,   458,   459,   428,   461,
     462,   463,   431,   465,   432,   466,   434,   467,   437,   436,
     446,   447,   468,   448,   469,   451,   472,   453,   452,   474,
     557,   454,   460,   486,   485,   489,   490,   464,   492,   493,
     494,   495,   496,   511,   513,   470,   514,   499,   501,   473,
     471,   504,   517,   518,   507,   484,   520,   510,   497,   522,
     475,   476,   477,   478,   479,   523,   532,   480,   481,   482,
     483,   488,   536,   491,   540,   502,   505,   508,   541,   512,
     525,   515,   543,   516,   519,   521,   524,   526,   527,   544,
     548,   552,   528,   529,   558,   559,   556,   530,   132,   560,
     561,   562,   563,   564,   565,   566,   531,   567,   542,   549,
     550,   551,   568,   553,   569,   554,   572,   573,   555,   578,
     581,   583,   570,   571,   585,   575,   574,   576,   594,   595,
     577,   597,   599,   602,   603,   579,   580,   693,   582,   604,
     605,   584,   606,   618,   623,   625,     0,   639,   586,   629,
     593,   596,   598,   630,   631,   600,   601,   617,   614,   619,
     632,   633,   634,   608,   610,   613,   620,   640,   621,   642,
     643,   622,   647,   648,   650,   652,   654,   661,   663,   624,
     626,   627,   635,   628,   665,   636,   667,   655,   669,   637,
     671,   638,   646,   675,   649,   684,   651,   653,   664,   656,
     687,   666,   668,   670,   688,   676,   703,   704,   680,   677,
     712,   681,   641,   682,   683,   685,   686,   689,   690,   691,
     714,   716,   718,   725,   726,   692,   727,   729,   730,   732,
     733,   734,   735,   736,   695,   696,   698,   741,   662,   742,
     700,   705,   702,   706,   711,   707,   739,   708,   740,   743,
     709,   744,   745,   679,   752,   753,   713,   715,   754,   755,
     756,   717,   757,     0,   719,   720,     0,   723,     0,     0,
     240,   724,   728,   731,   746,     0,     0,     0,     0,     0,
       0,     0,   747,   748,     0,     0,   749,   750,   751,     0,
     758,     0,     0,     0,   759,     0,     0,     0,     0,   385,
       0,     0,   710,     1,     0,     0,     0,     0,     0,     0,
     722,   318,     2,     3,     4,     5,     6,     7,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
       0,     0,    20,    21,    22,    23,    24,    25,    26,    27,
      28,     0,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,     0,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,     0,    71,    72,    73,    74,
      75,    76,     0,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    92,    93,
      94,    95,    96,    97,    98,    99,   100,   101,   102,     0,
     103,   104,   105,   106,   107,   108,   109,   110,   111,     0,
     396
};

static const yytype_int16 yycheck[] =
{
      16,    13,     3,     3,     3,     3,   134,     7,     3,     3,
       3,     3,     7,     7,     7,     7,     3,     3,     7,     3,
       7,   114,   134,     7,     3,     3,     3,     3,     7,     7,
       7,     3,     3,     3,     3,     3,   134,   118,   157,   577,
       7,   163,   164,   165,    70,    11,    12,    13,   167,     3,
     649,     3,   651,    79,   653,   593,   224,   225,   596,   134,
     153,     3,   230,   156,   232,     7,    92,     7,    11,    12,
     150,     3,    12,    13,    14,   101,   207,   208,     7,   105,
     106,    10,    11,    12,    13,    14,    11,    12,    13,   134,
      15,    16,     3,   173,   174,   134,     7,   696,   114,   698,
     116,     3,   118,   242,   243,    11,    12,    13,   189,    15,
      16,   233,    11,    12,    13,     3,    15,    16,    11,    12,
      13,     7,    15,    16,    10,    11,    12,     3,   135,   210,
     137,     7,   139,   252,   253,     3,   136,   138,   154,   138,
     138,   157,   158,   136,   136,   161,   162,   136,   134,   136,
     134,   167,   245,   246,     3,   134,   134,   134,   134,    11,
      12,    13,   134,   134,   134,   134,   134,   248,   134,   136,
       3,   251,     3,   189,   254,   135,   135,   137,   137,   139,
     139,   195,     3,   197,     3,   199,     7,     3,     7,     3,
     136,     7,     3,     7,   210,     3,     7,     3,     3,     7,
       3,   217,     7,     3,     7,     3,     3,     7,     3,     7,
      11,    12,    13,   161,   162,   134,     3,     3,     3,     3,
       3,   237,     3,   239,     3,     5,     3,     3,     3,   134,
     134,     5,   248,     3,     5,     3,   252,   253,   134,   134,
     134,     3,   134,     3,   134,   134,     3,   134,     0,   134,
     134,     7,     3,     7,   270,   135,   135,     7,     7,     7,
     276,     7,     7,     7,     7,     7,     7,   283,     7,     3,
     135,     7,   288,     3,     7,     7,   135,     4,     7,   135,
     135,     7,   136,     3,   135,   135,   134,   134,     7,   135,
     135,   135,   134,   134,     7,   135,     3,   135,   135,   135,
     135,   317,     7,   135,     7,   135,   135,   135,   135,   135,
     135,     7,     7,     7,   138,   138,     7,     7,     7,     3,
       7,     7,     7,     3,     7,     7,     7,   136,   135,     7,
       3,   135,     3,   135,     3,     3,   135,     3,     3,     3,
     136,   135,   135,   135,   135,   135,   135,   135,     7,     3,
       3,     7,     7,   135,     3,     7,     3,     3,   136,     3,
       3,     3,   135,     3,   136,     6,   135,     6,   134,   136,
     135,   135,     6,   136,     5,   136,     3,   135,   137,     3,
       5,   136,   135,     3,     7,     3,     3,   134,     3,     3,
       3,     3,     3,     3,     3,   135,     3,   413,   414,   136,
     135,   417,     3,     3,   420,   136,     3,   423,   136,     3,
     135,   135,   135,   135,   135,     7,     3,   135,   135,   135,
     135,   135,     3,   135,     3,   136,   136,   136,     3,   137,
     136,   135,     3,   135,   135,   135,   135,   135,   135,     3,
       3,     3,   135,   135,     3,     3,     7,   135,    13,     6,
       6,     6,     6,     3,     3,     3,   135,     3,   135,   135,
     135,   135,     3,   135,     3,   135,     3,     3,   135,     3,
       3,     3,   135,   135,     3,   135,   137,   135,     6,     6,
     137,     6,     3,     3,     3,   136,   136,    11,   136,     6,
       6,   136,     6,     3,     3,     3,    -1,     3,   136,     6,
     137,   137,   135,     6,     6,   137,   135,   137,   136,   135,
       6,     6,     6,   529,   530,   531,   137,     3,   137,     3,
       3,   137,     3,     3,     3,     3,     3,     3,     3,   137,
     135,   137,   135,   137,     3,   137,     3,   135,     3,   138,
       3,   137,   137,     3,   137,     3,   137,   137,   137,   135,
       6,   136,   136,   136,     3,   135,     3,     3,   135,   137,
       3,   135,   574,   135,   137,   135,   135,   135,   135,   135,
       3,     3,     3,     3,     3,   135,     3,     3,     3,     3,
       3,     3,     3,    11,   137,   137,   137,     3,   600,     3,
     137,   135,   137,   135,   137,   135,    11,   135,    11,     3,
     135,     3,     3,   619,     3,     3,   137,   137,     3,     3,
       3,   137,     3,    -1,   137,   137,    -1,   135,    -1,    -1,
      94,   136,   135,   135,   135,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   137,   137,    -1,    -1,   137,   137,   136,    -1,
     137,    -1,    -1,    -1,   137,    -1,    -1,    -1,    -1,   237,
      -1,    -1,   664,     8,    -1,    -1,    -1,    -1,    -1,    -1,
     676,   160,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      -1,    -1,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    -1,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    -1,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    -1,    91,    92,    93,    94,
      95,    96,    -1,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   118,   119,   120,   121,   122,   123,    -1,
     125,   126,   127,   128,   129,   130,   131,   132,   133,    -1,
     246
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     8,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    91,    92,    93,    94,    95,    96,    98,    99,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   109,   110,
     111,   112,   113,   114,   115,   116,   117,   118,   119,   120,
     121,   122,   123,   125,   126,   127,   128,   129,   130,   131,
     132,   133,   141,   148,   147,   134,   182,   134,   184,     3,
     134,     7,    10,    11,    12,   134,     3,   134,     7,    10,
      11,    12,    13,    14,   214,     3,   134,     7,    12,    14,
     214,     3,   134,   209,   210,     3,     7,   134,   208,   153,
     154,     3,     3,   151,   160,     3,   152,   161,   167,   166,
     169,   168,   165,   170,   171,   172,   134,   163,     3,     3,
       3,     3,     3,   149,   150,    11,    12,    13,   134,   212,
       3,   136,    11,    12,   213,     3,     3,     3,   134,   183,
       3,     3,     3,     3,   174,   186,     3,   175,     3,   176,
       5,     3,     3,     3,     3,   134,   134,   146,   157,   134,
     185,     3,     5,     3,    11,    12,    13,   212,     5,     3,
     134,   134,   134,   134,   180,   179,   212,     3,     3,   134,
     177,   134,   178,   173,   134,     3,   134,   145,    13,   212,
     209,   143,   144,   142,   212,   159,   158,   134,   181,   212,
     212,   155,   164,   162,   156,     0,     3,   193,     3,   134,
     188,   190,   192,   208,     7,     3,   187,   208,     7,   187,
     135,     7,     7,     7,     7,     7,   135,     7,     7,     7,
       7,     7,     3,   135,     7,     7,     7,     3,   135,     7,
       4,   135,     7,   136,     3,   189,   190,   191,   208,   191,
     135,   135,   190,     3,   197,   208,   135,   190,   197,     3,
     134,   200,   208,   201,   202,   203,   204,   205,   201,   200,
     200,   203,   203,   203,     7,   197,   135,   135,   135,   191,
     191,   134,   134,   134,     7,     3,     7,   136,   135,     3,
       3,     7,   136,   135,   135,   135,     7,   187,   135,   135,
     135,   206,   207,   206,   206,   206,   135,   135,   135,   135,
     135,     7,     7,   193,   193,     7,   187,   138,     3,   208,
     138,     7,     7,     7,     7,     3,   198,   198,     7,   198,
       7,   198,   203,     7,     7,   188,     3,   208,   134,   198,
     199,   199,   199,   190,   195,   196,   195,     7,   187,   191,
     197,   197,   191,   135,   135,     7,   136,   135,   135,   136,
     135,     3,   208,   135,   135,     3,   208,   135,     3,   208,
     135,     3,   208,   135,     3,   135,     3,   135,   136,     3,
       3,   135,   136,     3,   135,     7,   136,   134,     3,   208,
       3,     3,     3,     7,     7,     7,   135,   135,   136,     3,
     138,   136,   137,   135,   136,     3,   138,     3,     3,     3,
     135,     3,     3,     3,   134,     3,     6,     6,     6,     5,
     135,   135,     3,   136,     3,   135,   135,   135,   135,   135,
     135,   135,   135,   135,   136,     7,     3,   194,   135,     3,
       3,   135,     3,     3,     3,     3,     3,   136,     3,   208,
       3,   208,   136,     3,   208,   136,     3,   208,   136,     3,
     208,     3,   137,     3,     3,   135,   135,     3,     3,   135,
       3,   135,     3,     7,   135,   136,   135,   135,   135,   135,
     135,   135,     3,     3,     7,   136,     3,   135,   137,   139,
       3,     3,   135,     3,     3,   135,   137,   139,     3,   135,
     135,   135,     3,   135,   135,   135,     7,     5,     3,     3,
       6,     6,     6,     6,     3,     3,     3,     3,     3,     3,
     135,   135,     3,     3,   137,   135,   135,   137,     3,   136,
     136,     3,   136,     3,   136,     3,   136,    11,    12,    13,
      15,    16,   211,   137,     6,     6,   137,     6,   135,     3,
     137,   135,     3,     3,     6,     6,     6,   136,   208,   136,
     208,     3,   136,   208,   136,     3,   138,   137,     3,   135,
     137,   137,   137,     3,   137,     3,   135,   137,   137,     6,
       6,     6,     6,     6,     6,   135,   137,   138,   137,     3,
       3,   214,     3,     3,    15,   211,   137,     3,     3,   137,
       3,   137,     3,   137,     3,   135,   135,    15,   211,    15,
     211,     3,   214,     3,   137,     3,   136,     3,   136,     3,
     136,     3,   135,   137,   139,     3,   135,   137,     3,   208,
     135,   135,   135,   137,     3,   135,   135,     6,     3,   135,
     135,   135,   135,    11,   215,   137,   137,   215,   137,   215,
     137,   215,   137,     3,     3,   135,   135,   135,   135,   135,
     214,   137,     3,   137,     3,   137,     3,   137,     3,   137,
     137,     3,   208,   135,   136,     3,     3,     3,   135,     3,
       3,   135,     3,     3,     3,     3,    11,   215,   215,    11,
      11,     3,     3,     3,     3,     3,   135,   137,   137,   137,
     137,   136,     3,     3,     3,     3,     3,     3,   137,   137
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,   140,   141,   141,   141,   141,   141,   141,   141,   141,
     141,   141,   141,   141,   141,   141,   141,   141,   141,   141,
     141,   141,   141,   141,   141,   141,   141,   141,   141,   141,
     141,   141,   141,   141,   141,   141,   141,   141,   141,   141,
     141,   142,   141,   143,   141,   144,   141,   141,   141,   141,
     141,   141,   141,   141,   141,   141,   141,   141,   141,   141,
     141,   141,   141,   141,   141,   141,   141,   141,   141,   141,
     141,   141,   141,   141,   141,   141,   141,   141,   141,   141,
     141,   141,   141,   141,   141,   141,   141,   141,   141,   141,
     141,   145,   141,   146,   141,   147,   141,   148,   141,   149,
     141,   150,   141,   151,   141,   152,   141,   153,   141,   154,
     141,   155,   141,   156,   141,   157,   141,   158,   141,   159,
     141,   160,   141,   161,   141,   162,   141,   163,   141,   164,
     141,   141,   141,   141,   141,   141,   141,   141,   141,   165,
     141,   166,   141,   167,   141,   168,   141,   169,   141,   170,
     141,   171,   141,   172,   141,   173,   141,   141,   141,   141,
     141,   141,   141,   141,   141,   141,   141,   141,   141,   141,
     141,   141,   141,   141,   174,   141,   141,   175,   141,   141,
     176,   141,   141,   141,   141,   141,   141,   141,   141,   141,
     177,   141,   178,   141,   179,   141,   180,   141,   141,   141,
     141,   141,   141,   141,   141,   141,   141,   141,   181,   141,
     182,   141,   183,   141,   184,   141,   185,   141,   141,   141,
     141,   141,   141,   141,   141,   141,   141,   141,   141,   186,
     141,   141,   141,   141,   141,   187,   187,   187,   187,   188,
     188,   189,   189,   189,   190,   191,   191,   192,   192,   193,
     193,   194,   196,   195,   195,   197,   197,   197,   198,   199,
     199,   199,   200,   200,   200,   200,   202,   201,   201,   204,
     203,   205,   203,   207,   206,   208,   208,   210,   209,   211,
     211,   211,   211,   211,   212,   212,   212,   212,   213,   213,
     213,   214,   214,   215,   215
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     1,     1,     1,     1,     2,     3,     3,
       2,     3,     3,     3,     3,     3,     3,     3,     2,     8,
       8,     8,     9,     9,     9,     7,     4,     8,     8,     5,
       7,     8,     5,     5,     5,     5,     5,     5,     6,     5,
       3,     0,     3,     0,     3,     0,     3,     4,     4,     7,
       3,     5,     5,     5,     2,     2,     2,     3,     2,     2,
       2,     2,     2,     2,     2,     3,     3,     1,     1,     1,
       1,     2,     2,     2,     2,     1,     1,     1,     1,     3,
       8,     8,     7,    10,    11,     5,     7,     9,     9,     9,
       6,     0,     3,     0,     3,     0,     3,     0,     3,     0,
       3,     0,     3,     0,     3,     0,     3,     0,     3,     0,
       3,     0,     3,     0,     3,     0,     3,     0,     3,     0,
       3,     0,     3,     0,     3,     0,     3,     0,     3,     0,
       3,     4,     4,     4,     4,     8,     8,     8,     8,     0,
       3,     0,     3,     0,     3,     0,     3,     0,     3,     0,
       3,     0,     3,     0,     3,     0,     3,     3,     6,     9,
       9,     4,     6,     4,     6,     4,     6,     4,     6,     2,
       4,     2,     4,     2,     0,     3,     2,     0,     3,     2,
       0,     3,     3,     5,     8,     8,     8,     8,     8,     8,
       0,     3,     0,     3,     0,     3,     0,     3,     4,     4,
       5,     5,     5,     5,     5,     9,     9,     9,     0,     3,
       0,     3,     0,     3,     0,     3,     0,     3,     5,     6,
       6,     6,     6,     6,     6,     6,     6,     6,     6,     0,
       3,     4,     5,     5,     5,     3,     7,     7,     5,     3,
       7,     3,     7,     7,     4,     1,     1,     1,     1,     3,
       1,     3,     0,     2,     1,     3,     7,     7,     3,     4,
       6,     1,     4,     3,     7,     5,     0,     5,     1,     0,
       4,     0,     8,     0,     7,     0,     1,     0,     2,     1,
       1,     0,     1,     1,     0,     1,     1,     1,     0,     1,
       1,     0,     1,     0,     1
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
  YYSIZE_T yysize0 = yytnamerr (YY_NULLPTR, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTR;
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
                  YYSIZE_T yysize1 = yysize + yytnamerr (YY_NULLPTR, yytname[yyx]);
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
#line 179 "./config/rx-parse.y" /* yacc.c:1646  */
    { as_bad (_("Unknown opcode: %s"), rx_init_start); }
#line 2113 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 3:
#line 184 "./config/rx-parse.y" /* yacc.c:1646  */
    { B1 (0x00); }
#line 2119 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 4:
#line 187 "./config/rx-parse.y" /* yacc.c:1646  */
    { B1 (0x01); }
#line 2125 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 5:
#line 190 "./config/rx-parse.y" /* yacc.c:1646  */
    { B1 (0x02); }
#line 2131 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 6:
#line 193 "./config/rx-parse.y" /* yacc.c:1646  */
    { B1 (0x03); }
#line 2137 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 7:
#line 198 "./config/rx-parse.y" /* yacc.c:1646  */
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
#line 2155 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 8:
#line 213 "./config/rx-parse.y" /* yacc.c:1646  */
    { B1 (0x04); PC3 ((yyvsp[0].exp)); }
#line 2161 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 9:
#line 216 "./config/rx-parse.y" /* yacc.c:1646  */
    { B1 (0x08); rx_disp3 ((yyvsp[0].exp), 5); }
#line 2167 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 10:
#line 221 "./config/rx-parse.y" /* yacc.c:1646  */
    { if (rx_intop ((yyvsp[0].exp), 16, 16))
	      { B1 (0x39); PC2 ((yyvsp[0].exp)); }
	    else if (rx_intop ((yyvsp[0].exp), 24, 24))
	      { B1 (0x05); PC3 ((yyvsp[0].exp)); }
	    else
	      { rx_relax (RX_RELAX_BRANCH, 0);
		rx_linkrelax_branch ();
		B1 (0x39); PC2 ((yyvsp[0].exp)); } }
#line 2180 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 11:
#line 230 "./config/rx-parse.y" /* yacc.c:1646  */
    { B1 (0x05), PC3 ((yyvsp[0].exp)); }
#line 2186 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 12:
#line 235 "./config/rx-parse.y" /* yacc.c:1646  */
    { if ((yyvsp[-2].regno) == COND_EQ || (yyvsp[-2].regno) == COND_NE)
	      { B1 ((yyvsp[-2].regno) == COND_EQ ? 0x10 : 0x18); rx_disp3 ((yyvsp[0].exp), 5); }
	    else
	      as_bad (_("Only BEQ and BNE may have .S")); }
#line 2195 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 13:
#line 243 "./config/rx-parse.y" /* yacc.c:1646  */
    { B1 (0x20); F ((yyvsp[-2].regno), 4, 4); PC1 ((yyvsp[0].exp)); }
#line 2201 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 14:
#line 246 "./config/rx-parse.y" /* yacc.c:1646  */
    { B1 (0x2e), PC1 ((yyvsp[0].exp)); }
#line 2207 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 15:
#line 251 "./config/rx-parse.y" /* yacc.c:1646  */
    { B1 (0x38), PC2 ((yyvsp[0].exp)); }
#line 2213 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 16:
#line 253 "./config/rx-parse.y" /* yacc.c:1646  */
    { B1 (0x39), PC2 ((yyvsp[0].exp)); }
#line 2219 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 17:
#line 255 "./config/rx-parse.y" /* yacc.c:1646  */
    { if ((yyvsp[-2].regno) == COND_EQ || (yyvsp[-2].regno) == COND_NE)
	      { B1 ((yyvsp[-2].regno) == COND_EQ ? 0x3a : 0x3b); PC2 ((yyvsp[0].exp)); }
	    else
	      as_bad (_("Only BEQ and BNE may have .W")); }
#line 2228 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 18:
#line 260 "./config/rx-parse.y" /* yacc.c:1646  */
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
#line 2247 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 19:
#line 278 "./config/rx-parse.y" /* yacc.c:1646  */
    { B2 (0xf8, 0x04); F ((yyvsp[-1].regno), 8, 4); IMMB ((yyvsp[-4].exp), 12);}
#line 2253 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 20:
#line 281 "./config/rx-parse.y" /* yacc.c:1646  */
    { B2 (0xf8, 0x01); F ((yyvsp[-1].regno), 8, 4); IMMW ((yyvsp[-4].exp), 12);}
#line 2259 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 21:
#line 284 "./config/rx-parse.y" /* yacc.c:1646  */
    { B2 (0xf8, 0x02); F ((yyvsp[-1].regno), 8, 4); IMM ((yyvsp[-4].exp), 12);}
#line 2265 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 22:
#line 288 "./config/rx-parse.y" /* yacc.c:1646  */
    { if ((yyvsp[-1].regno) <= 7 && rx_uintop ((yyvsp[-5].exp), 8) && rx_disp5op0 (&(yyvsp[-3].exp), BSIZE))
	      { B2 (0x3c, 0); rx_field5s2 ((yyvsp[-3].exp)); F ((yyvsp[-1].regno), 9, 3); O1 ((yyvsp[-5].exp)); }
	    else
	      { B2 (0xf8, 0x04); F ((yyvsp[-1].regno), 8, 4); DSP ((yyvsp[-3].exp), 6, BSIZE); O1 ((yyvsp[-5].exp));
	      if ((yyvsp[-5].exp).X_op != O_constant && (yyvsp[-5].exp).X_op != O_big) rx_linkrelax_imm (12); } }
#line 2275 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 23:
#line 295 "./config/rx-parse.y" /* yacc.c:1646  */
    { if ((yyvsp[-1].regno) <= 7 && rx_uintop ((yyvsp[-5].exp), 8) && rx_disp5op0 (&(yyvsp[-3].exp), WSIZE))
	      { B2 (0x3d, 0); rx_field5s2 ((yyvsp[-3].exp)); F ((yyvsp[-1].regno), 9, 3); O1 ((yyvsp[-5].exp)); }
	    else
	      { B2 (0xf8, 0x01); F ((yyvsp[-1].regno), 8, 4); DSP ((yyvsp[-3].exp), 6, WSIZE); IMMW ((yyvsp[-5].exp), 12); } }
#line 2284 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 24:
#line 301 "./config/rx-parse.y" /* yacc.c:1646  */
    { if ((yyvsp[-1].regno) <= 7 && rx_uintop ((yyvsp[-5].exp), 8) && rx_disp5op0 (&(yyvsp[-3].exp), LSIZE))
	      { B2 (0x3e, 0); rx_field5s2 ((yyvsp[-3].exp)); F ((yyvsp[-1].regno), 9, 3); O1 ((yyvsp[-5].exp)); }
	    else
	      { B2 (0xf8, 0x02); F ((yyvsp[-1].regno), 8, 4); DSP ((yyvsp[-3].exp), 6, LSIZE); IMM ((yyvsp[-5].exp), 12); } }
#line 2293 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 25:
#line 309 "./config/rx-parse.y" /* yacc.c:1646  */
    { B2 (0x3f, 0); F ((yyvsp[-2].regno), 8, 4); F ((yyvsp[0].regno), 12, 4); rtsd_immediate ((yyvsp[-4].exp));
	    if ((yyvsp[-2].regno) == 0)
	      rx_error (_("RTSD cannot pop R0"));
	    if ((yyvsp[-2].regno) > (yyvsp[0].regno))
	      rx_error (_("RTSD first reg must be <= second reg")); }
#line 2303 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 26:
#line 318 "./config/rx-parse.y" /* yacc.c:1646  */
    { B2 (0x47, 0); F ((yyvsp[-2].regno), 8, 4); F ((yyvsp[0].regno), 12, 4); }
#line 2309 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 27:
#line 323 "./config/rx-parse.y" /* yacc.c:1646  */
    { B2 (0x44, 0); F ((yyvsp[-4].regno), 8, 4); F ((yyvsp[0].regno), 12, 4); DSP ((yyvsp[-6].exp), 6, BSIZE); }
#line 2315 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 28:
#line 326 "./config/rx-parse.y" /* yacc.c:1646  */
    { B3 (MEMEX, 0x04, 0); F ((yyvsp[-2].regno), 8, 2);  F ((yyvsp[-4].regno), 16, 4); F ((yyvsp[0].regno), 20, 4); DSP ((yyvsp[-6].exp), 14, sizemap[(yyvsp[-2].regno)]); }
#line 2321 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 29:
#line 331 "./config/rx-parse.y" /* yacc.c:1646  */
    { B2 (0x5b, 0x00); F ((yyvsp[-3].regno), 5, 1); F ((yyvsp[-2].regno), 8, 4); F ((yyvsp[0].regno), 12, 4); }
#line 2327 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 30:
#line 336 "./config/rx-parse.y" /* yacc.c:1646  */
    { B2 (0x58, 0x00); F ((yyvsp[-5].regno), 5, 1); F ((yyvsp[-3].regno), 8, 4); F ((yyvsp[0].regno), 12, 4); }
#line 2333 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 31:
#line 339 "./config/rx-parse.y" /* yacc.c:1646  */
    { if ((yyvsp[-3].regno) <= 7 && (yyvsp[0].regno) <= 7 && rx_disp5op (&(yyvsp[-5].exp), (yyvsp[-6].regno)))
	      { B2 (0xb0, 0); F ((yyvsp[-6].regno), 4, 1); F ((yyvsp[-3].regno), 9, 3); F ((yyvsp[0].regno), 13, 3); rx_field5s ((yyvsp[-5].exp)); }
	    else
	      { B2 (0x58, 0x00); F ((yyvsp[-6].regno), 5, 1); F ((yyvsp[-3].regno), 8, 4); F ((yyvsp[0].regno), 12, 4); DSP ((yyvsp[-5].exp), 6, (yyvsp[-6].regno)); } }
#line 2342 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 32:
#line 347 "./config/rx-parse.y" /* yacc.c:1646  */
    { if (rx_uintop ((yyvsp[-2].exp), 4))
	      { B2 (0x60, 0); FE ((yyvsp[-2].exp), 8, 4); F ((yyvsp[0].regno), 12, 4); }
	    else
	      /* This is really an add, but we negate the immediate.  */
	      { B2 (0x70, 0); F ((yyvsp[0].regno), 8, 4); F ((yyvsp[0].regno), 12, 4); NIMM ((yyvsp[-2].exp), 6); } }
#line 2352 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 33:
#line 354 "./config/rx-parse.y" /* yacc.c:1646  */
    { if (rx_uintop ((yyvsp[-2].exp), 4))
	      { B2 (0x61, 0); FE ((yyvsp[-2].exp), 8, 4); F ((yyvsp[0].regno), 12, 4); }
	    else if (rx_uintop ((yyvsp[-2].exp), 8))
	      { B2 (0x75, 0x50); F ((yyvsp[0].regno), 12, 4); UO1 ((yyvsp[-2].exp)); }
	    else
	      { B2 (0x74, 0x00); F ((yyvsp[0].regno), 12, 4); IMM ((yyvsp[-2].exp), 6); } }
#line 2363 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 34:
#line 362 "./config/rx-parse.y" /* yacc.c:1646  */
    { if (rx_uintop ((yyvsp[-2].exp), 4))
	      { B2 (0x62, 0); FE ((yyvsp[-2].exp), 8, 4); F ((yyvsp[0].regno), 12, 4); }
	    else
	      { B2 (0x70, 0); F ((yyvsp[0].regno), 8, 4); F ((yyvsp[0].regno), 12, 4); IMM ((yyvsp[-2].exp), 6); } }
#line 2372 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 35:
#line 368 "./config/rx-parse.y" /* yacc.c:1646  */
    { if (rx_uintop ((yyvsp[-2].exp), 4))
	      { B2 (0x63, 0); FE ((yyvsp[-2].exp), 8, 4); F ((yyvsp[0].regno), 12, 4); }
	    else
	      { B2 (0x74, 0x10); F ((yyvsp[0].regno), 12, 4); IMM ((yyvsp[-2].exp), 6); } }
#line 2381 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 36:
#line 374 "./config/rx-parse.y" /* yacc.c:1646  */
    { if (rx_uintop ((yyvsp[-2].exp), 4))
	      { B2 (0x64, 0); FE ((yyvsp[-2].exp), 8, 4); F ((yyvsp[0].regno), 12, 4); }
	    else
	      { B2 (0x74, 0x20); F ((yyvsp[0].regno), 12, 4); IMM ((yyvsp[-2].exp), 6); } }
#line 2390 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 37:
#line 380 "./config/rx-parse.y" /* yacc.c:1646  */
    { if (rx_uintop ((yyvsp[-2].exp), 4))
	      { B2 (0x65, 0); FE ((yyvsp[-2].exp), 8, 4); F ((yyvsp[0].regno), 12, 4); }
	    else
	      { B2 (0x74, 0x30); F ((yyvsp[0].regno), 12, 4); IMM ((yyvsp[-2].exp), 6); } }
#line 2399 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 38:
#line 386 "./config/rx-parse.y" /* yacc.c:1646  */
    { if (rx_uintop ((yyvsp[-2].exp), 4))
	      { B2 (0x66, 0); FE ((yyvsp[-2].exp), 8, 4); F ((yyvsp[0].regno), 12, 4); }
	    else if (rx_uintop ((yyvsp[-2].exp), 8))
	      { B2 (0x75, 0x40); F ((yyvsp[0].regno), 12, 4); UO1 ((yyvsp[-2].exp)); }
	    else
	      { B2 (0xfb, 0x02); F ((yyvsp[0].regno), 8, 4); IMM ((yyvsp[-2].exp), 12); } }
#line 2410 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 39:
#line 394 "./config/rx-parse.y" /* yacc.c:1646  */
    { if (rx_uintop ((yyvsp[-2].exp), 4))
	      { B2 (0x66, 0); FE ((yyvsp[-2].exp), 8, 4); F ((yyvsp[0].regno), 12, 4); }
	    else if (rx_uintop ((yyvsp[-2].exp), 8))
	      { B2 (0x75, 0x40); F ((yyvsp[0].regno), 12, 4); UO1 ((yyvsp[-2].exp)); }
	    else
	      { B2 (0xfb, 0x02); F ((yyvsp[0].regno), 8, 4); IMM ((yyvsp[-2].exp), 12); } }
#line 2421 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 40:
#line 404 "./config/rx-parse.y" /* yacc.c:1646  */
    { B1 (0x67); rtsd_immediate ((yyvsp[0].exp)); }
#line 2427 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 41:
#line 408 "./config/rx-parse.y" /* yacc.c:1646  */
    { sub_op = 0; }
#line 2433 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 43:
#line 409 "./config/rx-parse.y" /* yacc.c:1646  */
    { sub_op = 1; }
#line 2439 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 45:
#line 410 "./config/rx-parse.y" /* yacc.c:1646  */
    { sub_op = 2; }
#line 2445 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 47:
#line 415 "./config/rx-parse.y" /* yacc.c:1646  */
    {
	    if ((yyvsp[-2].regno) == (yyvsp[0].regno))
	      { B2 (0x7e, 0x80); F (LSIZE, 10, 2); F ((yyvsp[-2].regno), 12, 4); }
	    else
	     { B2 (0x6e, 0); F ((yyvsp[-2].regno), 8, 4); F ((yyvsp[0].regno), 12, 4); }
	    if ((yyvsp[-2].regno) == 0)
	      rx_error (_("PUSHM cannot push R0"));
	    if ((yyvsp[-2].regno) > (yyvsp[0].regno))
	      rx_error (_("PUSHM first reg must be <= second reg")); }
#line 2459 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 48:
#line 428 "./config/rx-parse.y" /* yacc.c:1646  */
    {
	    if ((yyvsp[-2].regno) == (yyvsp[0].regno))
	      { B2 (0x7e, 0xb0); F ((yyvsp[-2].regno), 12, 4); }
	    else
	      { B2 (0x6f, 0); F ((yyvsp[-2].regno), 8, 4); F ((yyvsp[0].regno), 12, 4); }
	    if ((yyvsp[-2].regno) == 0)
	      rx_error (_("POPM cannot pop R0"));
	    if ((yyvsp[-2].regno) > (yyvsp[0].regno))
	      rx_error (_("POPM first reg must be <= second reg")); }
#line 2473 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 49:
#line 441 "./config/rx-parse.y" /* yacc.c:1646  */
    { B2 (0x70, 0x00); F ((yyvsp[-2].regno), 8, 4); F ((yyvsp[0].regno), 12, 4); IMM ((yyvsp[-4].exp), 6); }
#line 2479 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 50:
#line 446 "./config/rx-parse.y" /* yacc.c:1646  */
    { B2(0x75, 0x60), UO1 ((yyvsp[0].exp)); }
#line 2485 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 51:
#line 451 "./config/rx-parse.y" /* yacc.c:1646  */
    { B2 (0x78, 0); FE ((yyvsp[-2].exp), 7, 5); F ((yyvsp[0].regno), 12, 4); }
#line 2491 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 52:
#line 453 "./config/rx-parse.y" /* yacc.c:1646  */
    { B2 (0x7a, 0); FE ((yyvsp[-2].exp), 7, 5); F ((yyvsp[0].regno), 12, 4); }
#line 2497 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 53:
#line 458 "./config/rx-parse.y" /* yacc.c:1646  */
    { B2 (0x7c, 0x00); FE ((yyvsp[-2].exp), 7, 5); F ((yyvsp[0].regno), 12, 4); }
#line 2503 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 54:
#line 463 "./config/rx-parse.y" /* yacc.c:1646  */
    { B2 (0x7e, 0x30); F ((yyvsp[0].regno), 12, 4); }
#line 2509 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 55:
#line 465 "./config/rx-parse.y" /* yacc.c:1646  */
    { B2 (0x7e, 0x40); F ((yyvsp[0].regno), 12, 4); }
#line 2515 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 56:
#line 467 "./config/rx-parse.y" /* yacc.c:1646  */
    { B2 (0x7e, 0x50); F ((yyvsp[0].regno), 12, 4); }
#line 2521 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 57:
#line 472 "./config/rx-parse.y" /* yacc.c:1646  */
    { B2 (0x7e, 0x80); F ((yyvsp[-1].regno), 10, 2); F ((yyvsp[0].regno), 12, 4); }
#line 2527 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 58:
#line 477 "./config/rx-parse.y" /* yacc.c:1646  */
    { B2 (0x7e, 0xb0); F ((yyvsp[0].regno), 12, 4); }
#line 2533 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 59:
#line 482 "./config/rx-parse.y" /* yacc.c:1646  */
    { if ((yyvsp[0].regno) == 13)
	      { rx_check_v2 (); }
	    if ((yyvsp[0].regno) < 16)
	      { B2 (0x7e, 0xc0); F ((yyvsp[0].regno), 12, 4); }
	    else
	      as_bad (_("PUSHC can only push the first 16 control registers")); }
#line 2544 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 60:
#line 492 "./config/rx-parse.y" /* yacc.c:1646  */
    { if ((yyvsp[0].regno) == 13)
	    { rx_check_v2 (); }
	    if ((yyvsp[0].regno) < 16)
	      { B2 (0x7e, 0xe0); F ((yyvsp[0].regno), 12, 4); }
	    else
	      as_bad (_("POPC can only pop the first 16 control registers")); }
#line 2555 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 61:
#line 502 "./config/rx-parse.y" /* yacc.c:1646  */
    { B2 (0x7f, 0xa0); F ((yyvsp[0].regno), 12, 4); }
#line 2561 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 62:
#line 504 "./config/rx-parse.y" /* yacc.c:1646  */
    { B2 (0x7f, 0xb0); F ((yyvsp[0].regno), 12, 4); }
#line 2567 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 63:
#line 509 "./config/rx-parse.y" /* yacc.c:1646  */
    { B2 (0x7f, 0x00); F ((yyvsp[0].regno), 12, 4); }
#line 2573 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 64:
#line 511 "./config/rx-parse.y" /* yacc.c:1646  */
    { B2 (0x7f, 0x10); F ((yyvsp[0].regno), 12, 4); }
#line 2579 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 65:
#line 513 "./config/rx-parse.y" /* yacc.c:1646  */
    { B2 (0x7f, 0x40); F ((yyvsp[0].regno), 12, 4); }
#line 2585 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 66:
#line 515 "./config/rx-parse.y" /* yacc.c:1646  */
    { B2 (0x7f, 0x50); F ((yyvsp[0].regno), 12, 4); }
#line 2591 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 67:
#line 520 "./config/rx-parse.y" /* yacc.c:1646  */
    { B2 (0x7f, 0x83); rx_note_string_insn_use (); }
#line 2597 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 68:
#line 522 "./config/rx-parse.y" /* yacc.c:1646  */
    { B2 (0x7f, 0x87); rx_note_string_insn_use (); }
#line 2603 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 69:
#line 524 "./config/rx-parse.y" /* yacc.c:1646  */
    { B2 (0x7f, 0x8b); rx_note_string_insn_use (); }
#line 2609 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 70:
#line 526 "./config/rx-parse.y" /* yacc.c:1646  */
    { B2 (0x7f, 0x8f); rx_note_string_insn_use (); }
#line 2615 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 71:
#line 531 "./config/rx-parse.y" /* yacc.c:1646  */
    { B2 (0x7f, 0x80); F ((yyvsp[0].regno), 14, 2); rx_note_string_insn_use (); }
#line 2621 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 72:
#line 533 "./config/rx-parse.y" /* yacc.c:1646  */
    { B2 (0x7f, 0x84); F ((yyvsp[0].regno), 14, 2); rx_note_string_insn_use (); }
#line 2627 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 73:
#line 535 "./config/rx-parse.y" /* yacc.c:1646  */
    { B2 (0x7f, 0x88); F ((yyvsp[0].regno), 14, 2); }
#line 2633 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 74:
#line 540 "./config/rx-parse.y" /* yacc.c:1646  */
    { B2 (0x7f, 0x8c); F ((yyvsp[0].regno), 14, 2); rx_note_string_insn_use (); }
#line 2639 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 75:
#line 545 "./config/rx-parse.y" /* yacc.c:1646  */
    { B2 (0x7f, 0x94); }
#line 2645 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 76:
#line 547 "./config/rx-parse.y" /* yacc.c:1646  */
    { B2 (0x7f, 0x95); }
#line 2651 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 77:
#line 549 "./config/rx-parse.y" /* yacc.c:1646  */
    { B2 (0x7f, 0x96); }
#line 2657 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 78:
#line 551 "./config/rx-parse.y" /* yacc.c:1646  */
    { B2 (0x7f, 0x93); }
#line 2663 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 79:
#line 556 "./config/rx-parse.y" /* yacc.c:1646  */
    { B3 (0x75, 0x70, 0x00); FE ((yyvsp[0].exp), 20, 4); }
#line 2669 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 80:
#line 562 "./config/rx-parse.y" /* yacc.c:1646  */
    { if ((yyvsp[-5].regno) <= 7 && (yyvsp[-1].regno) <= 7 && rx_disp5op (&(yyvsp[-3].exp), (yyvsp[-6].regno)))
	      { B2 (0x80, 0); F ((yyvsp[-6].regno), 2, 2); F ((yyvsp[-1].regno), 9, 3); F ((yyvsp[-5].regno), 13, 3); rx_field5s ((yyvsp[-3].exp)); }
	    else
	      { B2 (0xc3, 0x00); F ((yyvsp[-6].regno), 2, 2); F ((yyvsp[-1].regno), 8, 4); F ((yyvsp[-5].regno), 12, 4); DSP ((yyvsp[-3].exp), 4, (yyvsp[-6].regno)); }}
#line 2678 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 81:
#line 570 "./config/rx-parse.y" /* yacc.c:1646  */
    { if ((yyvsp[-3].regno) <= 7 && (yyvsp[0].regno) <= 7 && rx_disp5op (&(yyvsp[-5].exp), (yyvsp[-6].regno)))
	      { B2 (0x88, 0); F ((yyvsp[-6].regno), 2, 2); F ((yyvsp[-3].regno), 9, 3); F ((yyvsp[0].regno), 13, 3); rx_field5s ((yyvsp[-5].exp)); }
	    else
	      { B2 (0xcc, 0x00); F ((yyvsp[-6].regno), 2, 2); F ((yyvsp[-3].regno), 8, 4); F ((yyvsp[0].regno), 12, 4); DSP ((yyvsp[-5].exp), 6, (yyvsp[-6].regno)); } }
#line 2687 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 82:
#line 584 "./config/rx-parse.y" /* yacc.c:1646  */
    { B2 (0xc3, 0x00); F ((yyvsp[-5].regno), 2, 2); F ((yyvsp[-1].regno), 8, 4); F ((yyvsp[-4].regno), 12, 4); }
#line 2693 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 83:
#line 589 "./config/rx-parse.y" /* yacc.c:1646  */
    { B2 (0xc0, 0); F ((yyvsp[-8].regno), 2, 2); F ((yyvsp[-6].regno), 8, 4); F ((yyvsp[-1].regno), 12, 4); DSP ((yyvsp[-3].exp), 4, (yyvsp[-8].regno)); }
#line 2699 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 84:
#line 594 "./config/rx-parse.y" /* yacc.c:1646  */
    { B2 (0xc0, 0x00); F ((yyvsp[-9].regno), 2, 2); F ((yyvsp[-6].regno), 8, 4); F ((yyvsp[-1].regno), 12, 4); DSP ((yyvsp[-8].exp), 6, (yyvsp[-9].regno)); DSP ((yyvsp[-3].exp), 4, (yyvsp[-9].regno)); }
#line 2705 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 85:
#line 599 "./config/rx-parse.y" /* yacc.c:1646  */
    { B2 (0xcf, 0x00); F ((yyvsp[-3].regno), 2, 2); F ((yyvsp[-2].regno), 8, 4); F ((yyvsp[0].regno), 12, 4); }
#line 2711 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 86:
#line 604 "./config/rx-parse.y" /* yacc.c:1646  */
    { B2 (0xcc, 0x00); F ((yyvsp[-5].regno), 2, 2); F ((yyvsp[-3].regno), 8, 4); F ((yyvsp[0].regno), 12, 4); }
#line 2717 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 87:
#line 609 "./config/rx-parse.y" /* yacc.c:1646  */
    { B2 (0xf0, 0x00); F ((yyvsp[-2].regno), 8, 4); FE ((yyvsp[-6].exp), 13, 3); DSP ((yyvsp[-4].exp), 6, BSIZE); }
#line 2723 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 88:
#line 611 "./config/rx-parse.y" /* yacc.c:1646  */
    { B2 (0xf0, 0x08); F ((yyvsp[-2].regno), 8, 4); FE ((yyvsp[-6].exp), 13, 3); DSP ((yyvsp[-4].exp), 6, BSIZE); }
#line 2729 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 89:
#line 613 "./config/rx-parse.y" /* yacc.c:1646  */
    { B2 (0xf4, 0x00); F ((yyvsp[-2].regno), 8, 4); FE ((yyvsp[-6].exp), 13, 3); DSP ((yyvsp[-4].exp), 6, BSIZE); }
#line 2735 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 90:
#line 618 "./config/rx-parse.y" /* yacc.c:1646  */
    { B2 (0xf4, 0x08); F ((yyvsp[-4].regno), 14, 2); F ((yyvsp[-1].regno), 8, 4); DSP ((yyvsp[-3].exp), 6, (yyvsp[-4].regno)); }
#line 2741 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 91:
#line 622 "./config/rx-parse.y" /* yacc.c:1646  */
    { sub_op = 0; }
#line 2747 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 93:
#line 623 "./config/rx-parse.y" /* yacc.c:1646  */
    { sub_op = 1; sub_op2 = 1; }
#line 2753 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 95:
#line 624 "./config/rx-parse.y" /* yacc.c:1646  */
    { sub_op = 2; }
#line 2759 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 97:
#line 625 "./config/rx-parse.y" /* yacc.c:1646  */
    { sub_op = 3; sub_op2 = 2; }
#line 2765 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 99:
#line 626 "./config/rx-parse.y" /* yacc.c:1646  */
    { sub_op = 4; }
#line 2771 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 101:
#line 627 "./config/rx-parse.y" /* yacc.c:1646  */
    { sub_op = 5; }
#line 2777 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 103:
#line 628 "./config/rx-parse.y" /* yacc.c:1646  */
    { sub_op = 6; }
#line 2783 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 105:
#line 629 "./config/rx-parse.y" /* yacc.c:1646  */
    { sub_op = 7; }
#line 2789 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 107:
#line 630 "./config/rx-parse.y" /* yacc.c:1646  */
    { sub_op = 8; }
#line 2795 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 109:
#line 631 "./config/rx-parse.y" /* yacc.c:1646  */
    { sub_op = 9; }
#line 2801 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 111:
#line 632 "./config/rx-parse.y" /* yacc.c:1646  */
    { sub_op = 12; }
#line 2807 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 113:
#line 633 "./config/rx-parse.y" /* yacc.c:1646  */
    { sub_op = 13; }
#line 2813 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 115:
#line 634 "./config/rx-parse.y" /* yacc.c:1646  */
    { sub_op = 14; sub_op2 = 0; }
#line 2819 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 117:
#line 635 "./config/rx-parse.y" /* yacc.c:1646  */
    { sub_op = 14; sub_op2 = 0; }
#line 2825 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 119:
#line 636 "./config/rx-parse.y" /* yacc.c:1646  */
    { sub_op = 15; sub_op2 = 1; }
#line 2831 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 121:
#line 640 "./config/rx-parse.y" /* yacc.c:1646  */
    { sub_op = 6; }
#line 2837 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 123:
#line 641 "./config/rx-parse.y" /* yacc.c:1646  */
    { sub_op = 7; }
#line 2843 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 125:
#line 642 "./config/rx-parse.y" /* yacc.c:1646  */
    { sub_op = 16; }
#line 2849 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 127:
#line 643 "./config/rx-parse.y" /* yacc.c:1646  */
    { sub_op = 17; }
#line 2855 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 129:
#line 644 "./config/rx-parse.y" /* yacc.c:1646  */
    { sub_op = 21; }
#line 2861 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 131:
#line 649 "./config/rx-parse.y" /* yacc.c:1646  */
    { id24 (1, 0x63, 0x00); F ((yyvsp[0].regno), 16, 4); F ((yyvsp[-2].regno), 20, 4); }
#line 2867 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 132:
#line 651 "./config/rx-parse.y" /* yacc.c:1646  */
    { id24 (1, 0x67, 0x00); F ((yyvsp[0].regno), 16, 4); F ((yyvsp[-2].regno), 20, 4); }
#line 2873 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 133:
#line 653 "./config/rx-parse.y" /* yacc.c:1646  */
    { id24 (1, 0x6b, 0x00); F ((yyvsp[0].regno), 16, 4); F ((yyvsp[-2].regno), 20, 4); }
#line 2879 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 134:
#line 655 "./config/rx-parse.y" /* yacc.c:1646  */
    { id24 (1, 0x6f, 0x00); F ((yyvsp[0].regno), 16, 4); F ((yyvsp[-2].regno), 20, 4); }
#line 2885 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 135:
#line 658 "./config/rx-parse.y" /* yacc.c:1646  */
    { id24 (1, 0x60, 0x00); F ((yyvsp[-2].regno), 16, 4); F ((yyvsp[-6].regno), 20, 4); DSP ((yyvsp[-4].exp), 14, BSIZE); }
#line 2891 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 136:
#line 660 "./config/rx-parse.y" /* yacc.c:1646  */
    { id24 (1, 0x64, 0x00); F ((yyvsp[-2].regno), 16, 4); F ((yyvsp[-6].regno), 20, 4); DSP ((yyvsp[-4].exp), 14, BSIZE); }
#line 2897 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 137:
#line 662 "./config/rx-parse.y" /* yacc.c:1646  */
    { id24 (1, 0x68, 0x00); F ((yyvsp[-2].regno), 16, 4); F ((yyvsp[-6].regno), 20, 4); DSP ((yyvsp[-4].exp), 14, BSIZE); }
#line 2903 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 138:
#line 664 "./config/rx-parse.y" /* yacc.c:1646  */
    { id24 (1, 0x6c, 0x00); F ((yyvsp[-2].regno), 16, 4); F ((yyvsp[-6].regno), 20, 4); DSP ((yyvsp[-4].exp), 14, BSIZE); }
#line 2909 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 139:
#line 668 "./config/rx-parse.y" /* yacc.c:1646  */
    { sub_op = 0; }
#line 2915 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 141:
#line 669 "./config/rx-parse.y" /* yacc.c:1646  */
    { sub_op = 1; }
#line 2921 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 143:
#line 670 "./config/rx-parse.y" /* yacc.c:1646  */
    { sub_op = 2; }
#line 2927 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 145:
#line 671 "./config/rx-parse.y" /* yacc.c:1646  */
    { sub_op = 3; }
#line 2933 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 147:
#line 672 "./config/rx-parse.y" /* yacc.c:1646  */
    { sub_op = 4; }
#line 2939 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 149:
#line 673 "./config/rx-parse.y" /* yacc.c:1646  */
    { sub_op = 8; }
#line 2945 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 151:
#line 674 "./config/rx-parse.y" /* yacc.c:1646  */
    { sub_op = 5; }
#line 2951 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 153:
#line 675 "./config/rx-parse.y" /* yacc.c:1646  */
    { sub_op = 9; }
#line 2957 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 155:
#line 676 "./config/rx-parse.y" /* yacc.c:1646  */
    { sub_op = 6; }
#line 2963 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 157:
#line 684 "./config/rx-parse.y" /* yacc.c:1646  */
    { id24 (1, 0xdb, 0x00); F ((yyvsp[-2].regno), 20, 4); F ((yyvsp[0].regno), 16, 4); }
#line 2969 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 158:
#line 686 "./config/rx-parse.y" /* yacc.c:1646  */
    { id24 (1, 0xd0, 0x00); F ((yyvsp[-5].regno), 20, 4); F ((yyvsp[-4].regno), 12, 2); F ((yyvsp[-1].regno), 16, 4); DSP ((yyvsp[-3].exp), 14, (yyvsp[-4].regno)); }
#line 2975 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 159:
#line 691 "./config/rx-parse.y" /* yacc.c:1646  */
    { id24 (1, 0xe0, 0x00); F ((yyvsp[-8].regno), 20, 4); FE ((yyvsp[-6].exp), 11, 3);
	      F ((yyvsp[-2].regno), 16, 4); DSP ((yyvsp[-4].exp), 14, BSIZE); }
#line 2982 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 160:
#line 697 "./config/rx-parse.y" /* yacc.c:1646  */
    { id24 (1, 0xe0, 0x0f); FE ((yyvsp[-6].exp), 11, 3); F ((yyvsp[-2].regno), 16, 4);
	      DSP ((yyvsp[-4].exp), 14, BSIZE); }
#line 2989 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 161:
#line 703 "./config/rx-parse.y" /* yacc.c:1646  */
    { id24 (2, 0x00, 0x00); F ((yyvsp[-2].regno), 16, 4); F ((yyvsp[0].regno), 20, 4); }
#line 2995 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 162:
#line 705 "./config/rx-parse.y" /* yacc.c:1646  */
    { rx_check_v2 (); id24 (2, 0x00, 0x00); F ((yyvsp[-4].regno), 16, 4); F ((yyvsp[-2].regno), 20, 4); F ((yyvsp[0].regno), 12, 1); }
#line 3001 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 163:
#line 707 "./config/rx-parse.y" /* yacc.c:1646  */
    { id24 (2, 0x01, 0x00); F ((yyvsp[-2].regno), 16, 4); F ((yyvsp[0].regno), 20, 4); }
#line 3007 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 164:
#line 709 "./config/rx-parse.y" /* yacc.c:1646  */
    { rx_check_v2 (); id24 (2, 0x01, 0x00); F ((yyvsp[-4].regno), 16, 4); F ((yyvsp[-2].regno), 20, 4); F ((yyvsp[0].regno), 12, 1); }
#line 3013 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 165:
#line 711 "./config/rx-parse.y" /* yacc.c:1646  */
    { id24 (2, 0x04, 0x00); F ((yyvsp[-2].regno), 16, 4); F ((yyvsp[0].regno), 20, 4); }
#line 3019 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 166:
#line 713 "./config/rx-parse.y" /* yacc.c:1646  */
    { rx_check_v2 (); id24 (2, 0x04, 0x00); F ((yyvsp[-4].regno), 16, 4); F ((yyvsp[-2].regno), 20, 4); F ((yyvsp[0].regno), 12, 1); }
#line 3025 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 167:
#line 715 "./config/rx-parse.y" /* yacc.c:1646  */
    { id24 (2, 0x05, 0x00); F ((yyvsp[-2].regno), 16, 4); F ((yyvsp[0].regno), 20, 4); }
#line 3031 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 168:
#line 717 "./config/rx-parse.y" /* yacc.c:1646  */
    { rx_check_v2 (); id24 (2, 0x05, 0x00); F ((yyvsp[-4].regno), 16, 4); F ((yyvsp[-2].regno), 20, 4); F ((yyvsp[0].regno), 12, 1); }
#line 3037 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 169:
#line 723 "./config/rx-parse.y" /* yacc.c:1646  */
    { id24 (2, 0x17, 0x00); F ((yyvsp[0].regno), 20, 4); }
#line 3043 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 170:
#line 725 "./config/rx-parse.y" /* yacc.c:1646  */
    { rx_check_v2 (); id24 (2, 0x17, 0x00); F ((yyvsp[-2].regno), 20, 4); F ((yyvsp[0].regno), 16, 1); }
#line 3049 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 171:
#line 727 "./config/rx-parse.y" /* yacc.c:1646  */
    { id24 (2, 0x17, 0x10); F ((yyvsp[0].regno), 20, 4); }
#line 3055 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 172:
#line 729 "./config/rx-parse.y" /* yacc.c:1646  */
    { rx_check_v2 (); id24 (2, 0x17, 0x10); F ((yyvsp[-2].regno), 20, 4); F ((yyvsp[0].regno), 16, 1); }
#line 3061 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 173:
#line 731 "./config/rx-parse.y" /* yacc.c:1646  */
    { id24 (2, 0x1f, 0x00); F ((yyvsp[0].regno), 20, 4); }
#line 3067 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 174:
#line 732 "./config/rx-parse.y" /* yacc.c:1646  */
    { sub_op = 0; }
#line 3073 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 176:
#line 734 "./config/rx-parse.y" /* yacc.c:1646  */
    { id24 (2, 0x1f, 0x20); F ((yyvsp[0].regno), 20, 4); }
#line 3079 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 177:
#line 735 "./config/rx-parse.y" /* yacc.c:1646  */
    { sub_op = 2; }
#line 3085 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 179:
#line 737 "./config/rx-parse.y" /* yacc.c:1646  */
    { id24 (2, 0x1f, 0x10); F ((yyvsp[0].regno), 20, 4); }
#line 3091 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 180:
#line 738 "./config/rx-parse.y" /* yacc.c:1646  */
    { sub_op = 1; }
#line 3097 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 182:
#line 740 "./config/rx-parse.y" /* yacc.c:1646  */
    { id24 (2, 0x18, 0x00);
	    if (rx_uintop ((yyvsp[0].exp), 4) && (yyvsp[0].exp).X_add_number == 1)
	      ;
	    else if (rx_uintop ((yyvsp[0].exp), 4) && (yyvsp[0].exp).X_add_number == 2)
	      F (1, 19, 1);
	    else
	      as_bad (_("RACW expects #1 or #2"));}
#line 3109 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 183:
#line 748 "./config/rx-parse.y" /* yacc.c:1646  */
    { rx_check_v2 (); id24 (2, 0x18, 0x00); F ((yyvsp[0].regno), 16, 1);
	    if (rx_uintop ((yyvsp[-2].exp), 4) && (yyvsp[-2].exp).X_add_number == 1)
	      ;
	    else if (rx_uintop ((yyvsp[-2].exp), 4) && (yyvsp[-2].exp).X_add_number == 2)
	      F (1, 19, 1);
	    else
	      as_bad (_("RACW expects #1 or #2"));}
#line 3121 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 184:
#line 759 "./config/rx-parse.y" /* yacc.c:1646  */
    { id24 (2, 0x20, 0); F ((yyvsp[-6].regno), 14, 2); F ((yyvsp[-2].regno), 16, 4); F ((yyvsp[-5].regno), 20, 4); }
#line 3127 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 185:
#line 761 "./config/rx-parse.y" /* yacc.c:1646  */
    { id24 (2, 0x24, 0); F ((yyvsp[-6].regno), 14, 2); F ((yyvsp[-1].regno), 16, 4); F ((yyvsp[-5].regno), 20, 4); }
#line 3133 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 186:
#line 766 "./config/rx-parse.y" /* yacc.c:1646  */
    { id24 (2, 0x28, 0); F ((yyvsp[-6].regno), 14, 2); F ((yyvsp[-4].regno), 16, 4); F ((yyvsp[0].regno), 20, 4); }
#line 3139 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 187:
#line 768 "./config/rx-parse.y" /* yacc.c:1646  */
    { id24 (2, 0x2c, 0); F ((yyvsp[-6].regno), 14, 2); F ((yyvsp[-3].regno), 16, 4); F ((yyvsp[0].regno), 20, 4); }
#line 3145 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 188:
#line 773 "./config/rx-parse.y" /* yacc.c:1646  */
    { id24 (2, 0x38, 0); F ((yyvsp[-6].regno), 15, 1); F ((yyvsp[-4].regno), 16, 4); F ((yyvsp[0].regno), 20, 4); }
#line 3151 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 189:
#line 775 "./config/rx-parse.y" /* yacc.c:1646  */
    { id24 (2, 0x3c, 0); F ((yyvsp[-6].regno), 15, 1); F ((yyvsp[-3].regno), 16, 4); F ((yyvsp[0].regno), 20, 4); }
#line 3157 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 190:
#line 779 "./config/rx-parse.y" /* yacc.c:1646  */
    { sub_op = 6; }
#line 3163 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 192:
#line 780 "./config/rx-parse.y" /* yacc.c:1646  */
    { sub_op = 4; }
#line 3169 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 194:
#line 781 "./config/rx-parse.y" /* yacc.c:1646  */
    { sub_op = 5; }
#line 3175 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 196:
#line 782 "./config/rx-parse.y" /* yacc.c:1646  */
    { sub_op = 7; }
#line 3181 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 198:
#line 787 "./config/rx-parse.y" /* yacc.c:1646  */
    { if ((yyvsp[0].regno) == 13)
	      rx_check_v2 ();
	  id24 (2, 0x68, 0x00); F ((yyvsp[0].regno) % 16, 20, 4); F ((yyvsp[0].regno) / 16, 15, 1);
	    F ((yyvsp[-2].regno), 16, 4); }
#line 3190 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 199:
#line 795 "./config/rx-parse.y" /* yacc.c:1646  */
    { if ((yyvsp[-2].regno) == 13)
	    rx_check_v2 ();
	  id24 (2, 0x6a, 0); F ((yyvsp[-2].regno), 15, 5); F ((yyvsp[0].regno), 20, 4); }
#line 3198 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 200:
#line 802 "./config/rx-parse.y" /* yacc.c:1646  */
    { id24 (2, 0x6e, 0); FE ((yyvsp[-2].exp), 15, 5); F ((yyvsp[0].regno), 20, 4); }
#line 3204 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 201:
#line 804 "./config/rx-parse.y" /* yacc.c:1646  */
    { id24 (2, 0x6c, 0); FE ((yyvsp[-2].exp), 15, 5); F ((yyvsp[0].regno), 20, 4); }
#line 3210 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 202:
#line 809 "./config/rx-parse.y" /* yacc.c:1646  */
    { if ((yyvsp[0].regno) == 13)
	      rx_check_v2 ();
	    id24 (2, 0x73, 0x00); F ((yyvsp[0].regno), 19, 5); IMM ((yyvsp[-2].exp), 12); }
#line 3218 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 203:
#line 816 "./config/rx-parse.y" /* yacc.c:1646  */
    { id24 (2, 0xe0, 0x00); F ((yyvsp[-4].regno), 16, 4); FE ((yyvsp[-2].exp), 11, 5);
	      F ((yyvsp[0].regno), 20, 4); }
#line 3225 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 204:
#line 822 "./config/rx-parse.y" /* yacc.c:1646  */
    { id24 (2, 0xe0, 0xf0); FE ((yyvsp[-2].exp), 11, 5); F ((yyvsp[0].regno), 20, 4); }
#line 3231 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 205:
#line 827 "./config/rx-parse.y" /* yacc.c:1646  */
    { id24 (3, 0x00, 0); F ((yyvsp[-7].regno), 10, 2); F ((yyvsp[-3].regno), 12, 4); F ((yyvsp[-1].regno), 16, 4); F ((yyvsp[-6].regno), 20, 4); }
#line 3237 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 206:
#line 830 "./config/rx-parse.y" /* yacc.c:1646  */
    { id24 (3, 0x40, 0); F ((yyvsp[-7].regno), 10, 2); F ((yyvsp[-5].regno), 12, 4); F ((yyvsp[-3].regno), 16, 4); F ((yyvsp[0].regno), 20, 4); }
#line 3243 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 207:
#line 833 "./config/rx-parse.y" /* yacc.c:1646  */
    { id24 (3, 0xc0, 0); F ((yyvsp[-7].regno), 10, 2); F ((yyvsp[-5].regno), 12, 4); F ((yyvsp[-3].regno), 16, 4); F ((yyvsp[0].regno), 20, 4); }
#line 3249 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 208:
#line 837 "./config/rx-parse.y" /* yacc.c:1646  */
    { sub_op = 0; }
#line 3255 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 210:
#line 838 "./config/rx-parse.y" /* yacc.c:1646  */
    { sub_op = 2; }
#line 3261 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 212:
#line 839 "./config/rx-parse.y" /* yacc.c:1646  */
    { sub_op = 3; }
#line 3267 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 214:
#line 840 "./config/rx-parse.y" /* yacc.c:1646  */
    { sub_op = 4; }
#line 3273 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 216:
#line 841 "./config/rx-parse.y" /* yacc.c:1646  */
    { sub_op = 5; }
#line 3279 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 218:
#line 847 "./config/rx-parse.y" /* yacc.c:1646  */
    { id24 (2, 0x70, 0x20); F ((yyvsp[0].regno), 20, 4); NBIMM ((yyvsp[-2].exp), 12); }
#line 3285 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 219:
#line 852 "./config/rx-parse.y" /* yacc.c:1646  */
    { rx_check_v2 (); B3 (0xfd, 0x27, 0x00); F ((yyvsp[-1].regno), 16, 4); F ((yyvsp[-4].regno), 20, 4); }
#line 3291 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 220:
#line 857 "./config/rx-parse.y" /* yacc.c:1646  */
    { rx_check_v2 (); B3 (0xfd, 0x2f, 0x00); F ((yyvsp[-3].regno), 16, 4); F ((yyvsp[0].regno), 20, 4); }
#line 3297 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 221:
#line 862 "./config/rx-parse.y" /* yacc.c:1646  */
    { rx_check_v2 (); id24 (2, 0x07, 0x00); F ((yyvsp[-4].regno), 16, 4); F ((yyvsp[-2].regno), 20, 4); F ((yyvsp[0].regno), 12, 1); }
#line 3303 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 222:
#line 864 "./config/rx-parse.y" /* yacc.c:1646  */
    { rx_check_v2 (); id24 (2, 0x47, 0x00); F ((yyvsp[-4].regno), 16, 4); F ((yyvsp[-2].regno), 20, 4); F ((yyvsp[0].regno), 12, 1); }
#line 3309 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 223:
#line 866 "./config/rx-parse.y" /* yacc.c:1646  */
    { rx_check_v2 (); id24 (2, 0x03, 0x00); F ((yyvsp[-4].regno), 16, 4); F ((yyvsp[-2].regno), 20, 4); F ((yyvsp[0].regno), 12, 1); }
#line 3315 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 224:
#line 868 "./config/rx-parse.y" /* yacc.c:1646  */
    { rx_check_v2 (); id24 (2, 0x06, 0x00); F ((yyvsp[-4].regno), 16, 4); F ((yyvsp[-2].regno), 20, 4); F ((yyvsp[0].regno), 12, 1); }
#line 3321 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 225:
#line 870 "./config/rx-parse.y" /* yacc.c:1646  */
    { rx_check_v2 (); id24 (2, 0x44, 0x00); F ((yyvsp[-4].regno), 16, 4); F ((yyvsp[-2].regno), 20, 4); F ((yyvsp[0].regno), 12, 1); }
#line 3327 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 226:
#line 872 "./config/rx-parse.y" /* yacc.c:1646  */
    { rx_check_v2 (); id24 (2, 0x46, 0x00); F ((yyvsp[-4].regno), 16, 4); F ((yyvsp[-2].regno), 20, 4); F ((yyvsp[0].regno), 12, 1); }
#line 3333 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 227:
#line 874 "./config/rx-parse.y" /* yacc.c:1646  */
    { rx_check_v2 (); id24 (2, 0x45, 0x00); F ((yyvsp[-4].regno), 16, 4); F ((yyvsp[-2].regno), 20, 4); F ((yyvsp[0].regno), 12, 1); }
#line 3339 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 228:
#line 876 "./config/rx-parse.y" /* yacc.c:1646  */
    { rx_check_v2 (); id24 (2, 0x02, 0x00); F ((yyvsp[-4].regno), 16, 4); F ((yyvsp[-2].regno), 20, 4); F ((yyvsp[0].regno), 12, 1); }
#line 3345 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 229:
#line 877 "./config/rx-parse.y" /* yacc.c:1646  */
    { sub_op = 3; }
#line 3351 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 231:
#line 879 "./config/rx-parse.y" /* yacc.c:1646  */
    { rx_check_v2 (); id24 (2, 0x17, 0x30); F ((yyvsp[0].regno), 16, 1); F ((yyvsp[-2].regno), 20, 4); }
#line 3357 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 232:
#line 881 "./config/rx-parse.y" /* yacc.c:1646  */
    { rx_check_v2 (); id24 (2, 0x19, 0x00); F ((yyvsp[0].regno), 16, 1);
	    if (rx_uintop ((yyvsp[-2].exp), 4) && (yyvsp[-2].exp).X_add_number == 1)
	      ;
	    else if (rx_uintop ((yyvsp[-2].exp), 4) && (yyvsp[-2].exp).X_add_number == 2)
	      F (1, 19, 1);
	    else
	      as_bad (_("RACL expects #1 or #2"));}
#line 3369 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 233:
#line 889 "./config/rx-parse.y" /* yacc.c:1646  */
    { rx_check_v2 (); id24 (2, 0x19, 0x40); F ((yyvsp[0].regno), 16, 1);
	    if (rx_uintop ((yyvsp[-2].exp), 4) && (yyvsp[-2].exp).X_add_number == 1)
	      ;
	    else if (rx_uintop ((yyvsp[-2].exp), 4) && (yyvsp[-2].exp).X_add_number == 2)
	      F (1, 19, 1);
	    else
	      as_bad (_("RDACL expects #1 or #2"));}
#line 3381 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 234:
#line 897 "./config/rx-parse.y" /* yacc.c:1646  */
    { rx_check_v2 (); id24 (2, 0x18, 0x40); F ((yyvsp[0].regno), 16, 1);
	    if (rx_uintop ((yyvsp[-2].exp), 4) && (yyvsp[-2].exp).X_add_number == 1)
	      ;
	    else if (rx_uintop ((yyvsp[-2].exp), 4) && (yyvsp[-2].exp).X_add_number == 2)
	      F (1, 19, 1);
	    else
	      as_bad (_("RDACW expects #1 or #2"));}
#line 3393 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 235:
#line 913 "./config/rx-parse.y" /* yacc.c:1646  */
    { B2 (0x43 + (sub_op<<2), 0); F ((yyvsp[-2].regno), 8, 4); F ((yyvsp[0].regno), 12, 4); }
#line 3399 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 236:
#line 915 "./config/rx-parse.y" /* yacc.c:1646  */
    { B2 (0x40 + (sub_op<<2), 0); F ((yyvsp[-4].regno), 8, 4); F ((yyvsp[0].regno), 12, 4); DSP ((yyvsp[-6].exp), 6, BSIZE); }
#line 3405 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 237:
#line 917 "./config/rx-parse.y" /* yacc.c:1646  */
    { B3 (MEMEX, sub_op<<2, 0); F ((yyvsp[-2].regno), 8, 2); F ((yyvsp[-4].regno), 16, 4); F ((yyvsp[0].regno), 20, 4); DSP ((yyvsp[-6].exp), 14, sizemap[(yyvsp[-2].regno)]); }
#line 3411 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 238:
#line 919 "./config/rx-parse.y" /* yacc.c:1646  */
    { id24 (4, sub_op<<4, 0), F ((yyvsp[0].regno), 12, 4), F ((yyvsp[-4].regno), 16, 4), F ((yyvsp[-2].regno), 20, 4); }
#line 3417 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 239:
#line 926 "./config/rx-parse.y" /* yacc.c:1646  */
    { id24 (1, 0x03 + (sub_op<<2), 0x00); F ((yyvsp[-2].regno), 16, 4); F ((yyvsp[0].regno), 20, 4); }
#line 3423 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 240:
#line 928 "./config/rx-parse.y" /* yacc.c:1646  */
    { B4 (MEMEX, 0xa0, 0x00 + sub_op, 0x00);
	  F ((yyvsp[-4].regno), 24, 4); F ((yyvsp[0].regno), 28, 4); DSP ((yyvsp[-6].exp), 14, LSIZE); }
#line 3430 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 241:
#line 936 "./config/rx-parse.y" /* yacc.c:1646  */
    { id24 (1, 0x03 + (sub_op<<2), 0x00); F ((yyvsp[-2].regno), 16, 4); F ((yyvsp[0].regno), 20, 4); }
#line 3436 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 242:
#line 938 "./config/rx-parse.y" /* yacc.c:1646  */
    { id24 (1, 0x00 + (sub_op<<2), 0x00); F ((yyvsp[-4].regno), 16, 4); F ((yyvsp[0].regno), 20, 4); DSP ((yyvsp[-6].exp), 14, BSIZE); }
#line 3442 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 243:
#line 940 "./config/rx-parse.y" /* yacc.c:1646  */
    { B4 (MEMEX, 0x20 + ((yyvsp[-2].regno) << 6), 0x00 + sub_op, 0x00);
	  F ((yyvsp[-4].regno), 24, 4); F ((yyvsp[0].regno), 28, 4); DSP ((yyvsp[-6].exp), 14, sizemap[(yyvsp[-2].regno)]); }
#line 3449 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 244:
#line 946 "./config/rx-parse.y" /* yacc.c:1646  */
    { id24 (2, 0x70, sub_op<<4); F ((yyvsp[0].regno), 20, 4); IMM ((yyvsp[-2].exp), 12); }
#line 3455 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 249:
#line 961 "./config/rx-parse.y" /* yacc.c:1646  */
    { id24 (1, 0x03 + (sub_op<<2), 0x00); F ((yyvsp[-2].regno), 16, 4); F ((yyvsp[0].regno), 20, 4); }
#line 3461 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 250:
#line 963 "./config/rx-parse.y" /* yacc.c:1646  */
    { B2 (0x7e, sub_op2 << 4); F ((yyvsp[0].regno), 12, 4); }
#line 3467 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 251:
#line 968 "./config/rx-parse.y" /* yacc.c:1646  */
    { id24 (1, 0x4b + (sub_op2<<2), 0x00); F ((yyvsp[-2].regno), 16, 4); F ((yyvsp[0].regno), 20, 4); }
#line 3473 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 252:
#line 972 "./config/rx-parse.y" /* yacc.c:1646  */
    { rx_check_v2 (); }
#line 3479 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 255:
#line 980 "./config/rx-parse.y" /* yacc.c:1646  */
    { id24 (1, 0x03 + (sub_op<<2), 0); F ((yyvsp[-2].regno), 16, 4); F ((yyvsp[0].regno), 20, 4); }
#line 3485 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 256:
#line 982 "./config/rx-parse.y" /* yacc.c:1646  */
    { id24 (1, 0x00 + (sub_op<<2), 0); F ((yyvsp[-4].regno), 16, 4); F ((yyvsp[0].regno), 20, 4); DSP ((yyvsp[-6].exp), 14, BSIZE); }
#line 3491 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 257:
#line 984 "./config/rx-parse.y" /* yacc.c:1646  */
    { B4 (MEMEX, 0x20, 0x00 + sub_op, 0); F ((yyvsp[-2].regno), 8, 2); F ((yyvsp[-4].regno), 24, 4); F ((yyvsp[0].regno), 28, 4);
	    DSP ((yyvsp[-6].exp), 14, sizemap[(yyvsp[-2].regno)]); }
#line 3498 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 258:
#line 991 "./config/rx-parse.y" /* yacc.c:1646  */
    { id24 (2, 0x60 + sub_op, 0); F ((yyvsp[-2].regno), 16, 4); F ((yyvsp[0].regno), 20, 4); }
#line 3504 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 259:
#line 995 "./config/rx-parse.y" /* yacc.c:1646  */
    { B2 (0x68 + (sub_op<<1), 0); FE ((yyvsp[-2].exp), 7, 5); F ((yyvsp[0].regno), 12, 4); }
#line 3510 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 260:
#line 997 "./config/rx-parse.y" /* yacc.c:1646  */
    { id24 (2, 0x80 + (sub_op << 5), 0); FE ((yyvsp[-4].exp), 11, 5); F ((yyvsp[-2].regno), 16, 4); F ((yyvsp[0].regno), 20, 4); }
#line 3516 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 262:
#line 1003 "./config/rx-parse.y" /* yacc.c:1646  */
    { rx_check_float_support (); id24 (2, 0x72, sub_op << 4); F ((yyvsp[0].regno), 20, 4); O4 ((yyvsp[-2].exp)); }
#line 3522 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 263:
#line 1005 "./config/rx-parse.y" /* yacc.c:1646  */
    { rx_check_float_support (); id24 (1, 0x83 + (sub_op << 2), 0); F ((yyvsp[-2].regno), 16, 4); F ((yyvsp[0].regno), 20, 4); }
#line 3528 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 264:
#line 1007 "./config/rx-parse.y" /* yacc.c:1646  */
    { rx_check_float_support (); id24 (1, 0x80 + (sub_op << 2), 0); F ((yyvsp[-4].regno), 16, 4); F ((yyvsp[0].regno), 20, 4); DSP ((yyvsp[-6].exp), 14, LSIZE); }
#line 3534 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 265:
#line 1009 "./config/rx-parse.y" /* yacc.c:1646  */
    { rx_check_v2 (); id24 (4, 0x80 + (sub_op << 4), 0 ); F ((yyvsp[-4].regno), 16, 4); F ((yyvsp[-2].regno), 20, 4); F ((yyvsp[0].regno), 12, 4); }
#line 3540 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 266:
#line 1013 "./config/rx-parse.y" /* yacc.c:1646  */
    { rx_check_float_support (); }
#line 3546 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 267:
#line 1015 "./config/rx-parse.y" /* yacc.c:1646  */
    { id24 (2, 0x72, sub_op << 4); F ((yyvsp[0].regno), 20, 4); O4 ((yyvsp[-2].exp)); }
#line 3552 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 269:
#line 1020 "./config/rx-parse.y" /* yacc.c:1646  */
    { rx_check_float_support (); }
#line 3558 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 270:
#line 1022 "./config/rx-parse.y" /* yacc.c:1646  */
    { id24 (1, 0x83 + (sub_op << 2), 0); F ((yyvsp[-2].regno), 16, 4); F ((yyvsp[0].regno), 20, 4); }
#line 3564 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 271:
#line 1023 "./config/rx-parse.y" /* yacc.c:1646  */
    { rx_check_float_support (); }
#line 3570 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 272:
#line 1025 "./config/rx-parse.y" /* yacc.c:1646  */
    { id24 (1, 0x80 + (sub_op << 2), 0); F ((yyvsp[-4].regno), 16, 4); F ((yyvsp[0].regno), 20, 4); DSP ((yyvsp[-6].exp), 14, LSIZE); }
#line 3576 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 273:
#line 1029 "./config/rx-parse.y" /* yacc.c:1646  */
    { rx_check_v2 (); }
#line 3582 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 274:
#line 1031 "./config/rx-parse.y" /* yacc.c:1646  */
    { id24 (2, 0x1e, sub_op << 4); F ((yyvsp[0].regno), 20, 4); F ((yyvsp[-2].regno), 16, 1);
	    if (rx_uintop ((yyvsp[-4].exp), 4))
	      {
		switch (exp_val ((yyvsp[-4].exp)))
		  {
		  case 0:
		    F (1, 15, 1);
		    break;
		  case 1:
		    F (1, 15, 1);
		    F (1, 17, 1);
		    break;
		  case 2:
		    break;
		  default:
		    as_bad (_("IMM expects #0 to #2"));}
	      } else
	        as_bad (_("IMM expects #0 to #2"));}
#line 3605 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 275:
#line 1053 "./config/rx-parse.y" /* yacc.c:1646  */
    { (yyval.exp) = zero_expr (); }
#line 3611 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 276:
#line 1054 "./config/rx-parse.y" /* yacc.c:1646  */
    { (yyval.exp) = (yyvsp[0].exp); }
#line 3617 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 277:
#line 1057 "./config/rx-parse.y" /* yacc.c:1646  */
    { need_flag = 1; }
#line 3623 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 278:
#line 1057 "./config/rx-parse.y" /* yacc.c:1646  */
    { need_flag = 0; (yyval.regno) = (yyvsp[0].regno); }
#line 3629 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 279:
#line 1062 "./config/rx-parse.y" /* yacc.c:1646  */
    { (yyval.regno) = 0; }
#line 3635 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 280:
#line 1063 "./config/rx-parse.y" /* yacc.c:1646  */
    { (yyval.regno) = 1; }
#line 3641 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 281:
#line 1064 "./config/rx-parse.y" /* yacc.c:1646  */
    { (yyval.regno) = 2; }
#line 3647 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 282:
#line 1065 "./config/rx-parse.y" /* yacc.c:1646  */
    { (yyval.regno) = 2; }
#line 3653 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 283:
#line 1066 "./config/rx-parse.y" /* yacc.c:1646  */
    { (yyval.regno) = 3; }
#line 3659 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 284:
#line 1069 "./config/rx-parse.y" /* yacc.c:1646  */
    { (yyval.regno) = LSIZE; }
#line 3665 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 285:
#line 1070 "./config/rx-parse.y" /* yacc.c:1646  */
    { (yyval.regno) = BSIZE; }
#line 3671 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 286:
#line 1071 "./config/rx-parse.y" /* yacc.c:1646  */
    { (yyval.regno) = WSIZE; }
#line 3677 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 287:
#line 1072 "./config/rx-parse.y" /* yacc.c:1646  */
    { (yyval.regno) = LSIZE; }
#line 3683 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 288:
#line 1075 "./config/rx-parse.y" /* yacc.c:1646  */
    { (yyval.regno) = 1; }
#line 3689 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 289:
#line 1076 "./config/rx-parse.y" /* yacc.c:1646  */
    { (yyval.regno) = 0; }
#line 3695 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 290:
#line 1077 "./config/rx-parse.y" /* yacc.c:1646  */
    { (yyval.regno) = 1; }
#line 3701 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 291:
#line 1080 "./config/rx-parse.y" /* yacc.c:1646  */
    {}
#line 3707 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 292:
#line 1081 "./config/rx-parse.y" /* yacc.c:1646  */
    {}
#line 3713 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 293:
#line 1084 "./config/rx-parse.y" /* yacc.c:1646  */
    {}
#line 3719 "rx-parse.c" /* yacc.c:1646  */
    break;

  case 294:
#line 1085 "./config/rx-parse.y" /* yacc.c:1646  */
    {}
#line 3725 "rx-parse.c" /* yacc.c:1646  */
    break;


#line 3729 "rx-parse.c" /* yacc.c:1646  */
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
#line 1088 "./config/rx-parse.y" /* yacc.c:1906  */

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
  { "extb", CREG, 13 },

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

  { "a0", ACC, 0 },
  { "a1", ACC, 1 },

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
  OPC(EMACA),
  OPC(EMSBA),
  OPC(EMUL),
  OPC(EMULA),
  OPC(EMULU),
  OPC(FADD),
  OPC(FCMP),
  OPC(FDIV),
  OPC(FMUL),
  OPC(FREIT),
  OPC(FSQRT),
  OPC(FTOU),
  OPC(FSUB),
  OPC(FTOI),
  OPC(INT),
  OPC(ITOF),
  OPC(JMP),
  OPC(JSR),
  OPC(MVFACGU),
  OPC(MVFACHI),
  OPC(MVFACMI),
  OPC(MVFACLO),
  OPC(MVFC),
  OPC(MVTACGU),
  OPC(MVTACHI),
  OPC(MVTACLO),
  OPC(MVTC),
  OPC(MVTIPL),
  OPC(MACHI),
  OPC(MACLO),
  OPC(MACLH),
  OPC(MAX),
  OPC(MIN),
  OPC(MOV),
  OPC(MOVCO),
  OPC(MOVLI),
  OPC(MOVU),
  OPC(MSBHI),
  OPC(MSBLH),
  OPC(MSBLO),
  OPC(MUL),
  OPC(MULHI),
  OPC(MULLH),
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
  OPC(RACL),
  OPC(RACW),
  OPC(RDACL),
  OPC(RDACW),
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
  OPC(UTOF),
  OPC(WAIT),
  OPC(XCHG),
  OPC(XOR),
};

#define NUM_TOKENS (sizeof (token_table) / sizeof (token_table[0]))

static struct
{
  const char * string;
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
  const char * string;
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
check_condition (const char * base)
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

  if (exp.X_op == O_big)
    {
      if (nbits == 32)
	return 1;
      if (exp.X_add_number == -1)
	return 0;
    }
  else if (exp.X_op != O_constant)
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
  /* We will emit constants ourselves here, so negate them.  */
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

static void
rx_check_v2 (void)
{
  if (rx_cpu < RXV2)
    rx_error (_("target CPU type does not support v2 instructions"));
}
