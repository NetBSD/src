/* A Bison parser, made by GNU Bison 3.0.5.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018 Free Software Foundation, Inc.

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
#define YYBISON_VERSION "3.0.5"

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
#define DSIZE 3

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

#define POST(v)            rx_post (v)

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
static void        rx_check_v3 (void);
static void        rx_check_dfpu (void);

static int    need_flag = 0;
static int    rx_in_brackets = 0;
static int    rx_last_token = 0;
static char * rx_init_start;
static char * rx_last_exp_start = 0;
static int    sub_op;
static int    sub_op2;

#define YYDEBUG 1
#define YYERROR_VERBOSE 1


#line 192 "rx-parse.c" /* yacc.c:339  */

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
    DREG = 262,
    DREGH = 263,
    DREGL = 264,
    DCREG = 265,
    EXPR = 266,
    UNKNOWN_OPCODE = 267,
    IS_OPCODE = 268,
    DOT_S = 269,
    DOT_B = 270,
    DOT_W = 271,
    DOT_L = 272,
    DOT_A = 273,
    DOT_UB = 274,
    DOT_UW = 275,
    DOT_D = 276,
    ABS = 277,
    ADC = 278,
    ADD = 279,
    AND_ = 280,
    BCLR = 281,
    BCND = 282,
    BFMOV = 283,
    BFMOVZ = 284,
    BMCND = 285,
    BNOT = 286,
    BRA = 287,
    BRK = 288,
    BSET = 289,
    BSR = 290,
    BTST = 291,
    CLRPSW = 292,
    CMP = 293,
    DABS = 294,
    DADD = 295,
    DBT = 296,
    DCMP = 297,
    DDIV = 298,
    DIV = 299,
    DIVU = 300,
    DMOV = 301,
    DMUL = 302,
    DNEG = 303,
    DPOPM = 304,
    DPUSHM = 305,
    DROUND = 306,
    DSQRT = 307,
    DSUB = 308,
    DTOF = 309,
    DTOI = 310,
    DTOU = 311,
    EDIV = 312,
    EDIVU = 313,
    EMACA = 314,
    EMSBA = 315,
    EMUL = 316,
    EMULA = 317,
    EMULU = 318,
    FADD = 319,
    FCMP = 320,
    FDIV = 321,
    FMUL = 322,
    FREIT = 323,
    FSUB = 324,
    FSQRT = 325,
    FTOD = 326,
    FTOI = 327,
    FTOU = 328,
    INT = 329,
    ITOD = 330,
    ITOF = 331,
    JMP = 332,
    JSR = 333,
    MACHI = 334,
    MACLH = 335,
    MACLO = 336,
    MAX = 337,
    MIN = 338,
    MOV = 339,
    MOVCO = 340,
    MOVLI = 341,
    MOVU = 342,
    MSBHI = 343,
    MSBLH = 344,
    MSBLO = 345,
    MUL = 346,
    MULHI = 347,
    MULLH = 348,
    MULLO = 349,
    MULU = 350,
    MVFACHI = 351,
    MVFACGU = 352,
    MVFACMI = 353,
    MVFACLO = 354,
    MVFC = 355,
    MVFDC = 356,
    MVFDR = 357,
    MVTACGU = 358,
    MVTACHI = 359,
    MVTACLO = 360,
    MVTC = 361,
    MVTDC = 362,
    MVTIPL = 363,
    NEG = 364,
    NOP = 365,
    NOT = 366,
    OR = 367,
    POP = 368,
    POPC = 369,
    POPM = 370,
    PUSH = 371,
    PUSHA = 372,
    PUSHC = 373,
    PUSHM = 374,
    RACL = 375,
    RACW = 376,
    RDACL = 377,
    RDACW = 378,
    REIT = 379,
    REVL = 380,
    REVW = 381,
    RMPA = 382,
    ROLC = 383,
    RORC = 384,
    ROTL = 385,
    ROTR = 386,
    ROUND = 387,
    RSTR = 388,
    RTE = 389,
    RTFI = 390,
    RTS = 391,
    RTSD = 392,
    SAT = 393,
    SATR = 394,
    SAVE = 395,
    SBB = 396,
    SCCND = 397,
    SCMPU = 398,
    SETPSW = 399,
    SHAR = 400,
    SHLL = 401,
    SHLR = 402,
    SMOVB = 403,
    SMOVF = 404,
    SMOVU = 405,
    SSTR = 406,
    STNZ = 407,
    STOP = 408,
    STZ = 409,
    SUB = 410,
    SUNTIL = 411,
    SWHILE = 412,
    TST = 413,
    UTOD = 414,
    UTOF = 415,
    WAIT = 416,
    XCHG = 417,
    XOR = 418
  };
#endif
/* Tokens.  */
#define REG 258
#define FLAG 259
#define CREG 260
#define ACC 261
#define DREG 262
#define DREGH 263
#define DREGL 264
#define DCREG 265
#define EXPR 266
#define UNKNOWN_OPCODE 267
#define IS_OPCODE 268
#define DOT_S 269
#define DOT_B 270
#define DOT_W 271
#define DOT_L 272
#define DOT_A 273
#define DOT_UB 274
#define DOT_UW 275
#define DOT_D 276
#define ABS 277
#define ADC 278
#define ADD 279
#define AND_ 280
#define BCLR 281
#define BCND 282
#define BFMOV 283
#define BFMOVZ 284
#define BMCND 285
#define BNOT 286
#define BRA 287
#define BRK 288
#define BSET 289
#define BSR 290
#define BTST 291
#define CLRPSW 292
#define CMP 293
#define DABS 294
#define DADD 295
#define DBT 296
#define DCMP 297
#define DDIV 298
#define DIV 299
#define DIVU 300
#define DMOV 301
#define DMUL 302
#define DNEG 303
#define DPOPM 304
#define DPUSHM 305
#define DROUND 306
#define DSQRT 307
#define DSUB 308
#define DTOF 309
#define DTOI 310
#define DTOU 311
#define EDIV 312
#define EDIVU 313
#define EMACA 314
#define EMSBA 315
#define EMUL 316
#define EMULA 317
#define EMULU 318
#define FADD 319
#define FCMP 320
#define FDIV 321
#define FMUL 322
#define FREIT 323
#define FSUB 324
#define FSQRT 325
#define FTOD 326
#define FTOI 327
#define FTOU 328
#define INT 329
#define ITOD 330
#define ITOF 331
#define JMP 332
#define JSR 333
#define MACHI 334
#define MACLH 335
#define MACLO 336
#define MAX 337
#define MIN 338
#define MOV 339
#define MOVCO 340
#define MOVLI 341
#define MOVU 342
#define MSBHI 343
#define MSBLH 344
#define MSBLO 345
#define MUL 346
#define MULHI 347
#define MULLH 348
#define MULLO 349
#define MULU 350
#define MVFACHI 351
#define MVFACGU 352
#define MVFACMI 353
#define MVFACLO 354
#define MVFC 355
#define MVFDC 356
#define MVFDR 357
#define MVTACGU 358
#define MVTACHI 359
#define MVTACLO 360
#define MVTC 361
#define MVTDC 362
#define MVTIPL 363
#define NEG 364
#define NOP 365
#define NOT 366
#define OR 367
#define POP 368
#define POPC 369
#define POPM 370
#define PUSH 371
#define PUSHA 372
#define PUSHC 373
#define PUSHM 374
#define RACL 375
#define RACW 376
#define RDACL 377
#define RDACW 378
#define REIT 379
#define REVL 380
#define REVW 381
#define RMPA 382
#define ROLC 383
#define RORC 384
#define ROTL 385
#define ROTR 386
#define ROUND 387
#define RSTR 388
#define RTE 389
#define RTFI 390
#define RTS 391
#define RTSD 392
#define SAT 393
#define SATR 394
#define SAVE 395
#define SBB 396
#define SCCND 397
#define SCMPU 398
#define SETPSW 399
#define SHAR 400
#define SHLL 401
#define SHLR 402
#define SMOVB 403
#define SMOVF 404
#define SMOVU 405
#define SSTR 406
#define STNZ 407
#define STOP 408
#define STZ 409
#define SUB 410
#define SUNTIL 411
#define SWHILE 412
#define TST 413
#define UTOD 414
#define UTOF 415
#define WAIT 416
#define XCHG 417
#define XOR 418

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 140 "./config/rx-parse.y" /* yacc.c:355  */

  int regno;
  expressionS exp;

#line 563 "rx-parse.c" /* yacc.c:355  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE rx_lval;

int rx_parse (void);

#endif /* !YY_RX_RX_PARSE_H_INCLUDED  */

/* Copy the second part of user declarations.  */

#line 580 "rx-parse.c" /* yacc.c:358  */

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
#define YYFINAL  307
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   967

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  170
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  97
/* YYNRULES -- Number of rules.  */
#define YYNRULES  356
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  924

/* YYTRANSLATE[YYX] -- Symbol number corresponding to YYX as returned
   by yylex, with out-of-bounds checking.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   418

#define YYTRANSLATE(YYX)                                                \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, without out-of-bounds checking.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,   164,     2,     2,     2,     2,
       2,     2,     2,   169,   165,   168,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   166,     2,   167,     2,     2,     2,     2,     2,     2,
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
     125,   126,   127,   128,   129,   130,   131,   132,   133,   134,
     135,   136,   137,   138,   139,   140,   141,   142,   143,   144,
     145,   146,   147,   148,   149,   150,   151,   152,   153,   154,
     155,   156,   157,   158,   159,   160,   161,   162,   163
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   184,   184,   189,   192,   195,   198,   203,   218,   221,
     226,   235,   240,   248,   251,   256,   258,   260,   265,   283,
     286,   289,   292,   300,   306,   314,   323,   328,   331,   336,
     341,   344,   352,   359,   367,   373,   379,   385,   391,   399,
     409,   414,   414,   415,   415,   416,   416,   420,   433,   446,
     451,   456,   458,   463,   468,   470,   472,   477,   482,   487,
     497,   507,   509,   514,   516,   518,   520,   525,   527,   529,
     531,   536,   538,   540,   545,   550,   552,   554,   556,   561,
     567,   575,   589,   594,   599,   604,   609,   614,   616,   618,
     623,   628,   628,   629,   629,   630,   630,   631,   631,   632,
     632,   633,   633,   634,   634,   635,   635,   636,   636,   637,
     637,   638,   638,   639,   639,   640,   640,   641,   641,   642,
     642,   646,   646,   647,   647,   648,   648,   649,   649,   650,
     650,   654,   656,   658,   660,   663,   665,   667,   669,   674,
     674,   675,   675,   676,   676,   677,   677,   678,   678,   679,
     679,   680,   680,   681,   681,   682,   682,   689,   691,   696,
     702,   708,   710,   712,   714,   716,   718,   720,   722,   728,
     730,   732,   734,   736,   738,   738,   739,   741,   741,   742,
     744,   744,   745,   753,   764,   766,   771,   773,   778,   780,
     785,   785,   786,   786,   787,   787,   788,   788,   792,   800,
     807,   809,   814,   821,   827,   832,   835,   838,   843,   843,
     844,   844,   845,   845,   846,   846,   847,   847,   852,   857,
     862,   867,   869,   871,   873,   875,   877,   879,   881,   883,
     883,   884,   886,   894,   902,   912,   912,   913,   913,   916,
     916,   917,   917,   920,   920,   921,   921,   922,   922,   923,
     923,   924,   924,   925,   925,   926,   926,   927,   927,   928,
     928,   929,   929,   930,   930,   931,   933,   936,   939,   942,
     945,   948,   951,   954,   958,   961,   965,   968,   971,   974,
     977,   980,   983,   986,   989,   991,   994,   997,  1000,  1011,
    1013,  1015,  1017,  1024,  1026,  1034,  1036,  1038,  1044,  1049,
    1050,  1054,  1055,  1059,  1061,  1066,  1071,  1071,  1073,  1078,
    1080,  1082,  1089,  1093,  1095,  1097,  1101,  1103,  1105,  1107,
    1112,  1112,  1115,  1119,  1119,  1122,  1122,  1128,  1128,  1151,
    1152,  1157,  1157,  1165,  1167,  1172,  1176,  1181,  1182,  1185,
    1185,  1190,  1191,  1192,  1193,  1194,  1197,  1198,  1199,  1200,
    1203,  1204,  1205,  1208,  1209,  1212,  1213
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "REG", "FLAG", "CREG", "ACC", "DREG",
  "DREGH", "DREGL", "DCREG", "EXPR", "UNKNOWN_OPCODE", "IS_OPCODE",
  "DOT_S", "DOT_B", "DOT_W", "DOT_L", "DOT_A", "DOT_UB", "DOT_UW", "DOT_D",
  "ABS", "ADC", "ADD", "AND_", "BCLR", "BCND", "BFMOV", "BFMOVZ", "BMCND",
  "BNOT", "BRA", "BRK", "BSET", "BSR", "BTST", "CLRPSW", "CMP", "DABS",
  "DADD", "DBT", "DCMP", "DDIV", "DIV", "DIVU", "DMOV", "DMUL", "DNEG",
  "DPOPM", "DPUSHM", "DROUND", "DSQRT", "DSUB", "DTOF", "DTOI", "DTOU",
  "EDIV", "EDIVU", "EMACA", "EMSBA", "EMUL", "EMULA", "EMULU", "FADD",
  "FCMP", "FDIV", "FMUL", "FREIT", "FSUB", "FSQRT", "FTOD", "FTOI", "FTOU",
  "INT", "ITOD", "ITOF", "JMP", "JSR", "MACHI", "MACLH", "MACLO", "MAX",
  "MIN", "MOV", "MOVCO", "MOVLI", "MOVU", "MSBHI", "MSBLH", "MSBLO", "MUL",
  "MULHI", "MULLH", "MULLO", "MULU", "MVFACHI", "MVFACGU", "MVFACMI",
  "MVFACLO", "MVFC", "MVFDC", "MVFDR", "MVTACGU", "MVTACHI", "MVTACLO",
  "MVTC", "MVTDC", "MVTIPL", "NEG", "NOP", "NOT", "OR", "POP", "POPC",
  "POPM", "PUSH", "PUSHA", "PUSHC", "PUSHM", "RACL", "RACW", "RDACL",
  "RDACW", "REIT", "REVL", "REVW", "RMPA", "ROLC", "RORC", "ROTL", "ROTR",
  "ROUND", "RSTR", "RTE", "RTFI", "RTS", "RTSD", "SAT", "SATR", "SAVE",
  "SBB", "SCCND", "SCMPU", "SETPSW", "SHAR", "SHLL", "SHLR", "SMOVB",
  "SMOVF", "SMOVU", "SSTR", "STNZ", "STOP", "STZ", "SUB", "SUNTIL",
  "SWHILE", "TST", "UTOD", "UTOF", "WAIT", "XCHG", "XOR", "'#'", "','",
  "'['", "']'", "'-'", "'+'", "$accept", "statement", "$@1", "$@2", "$@3",
  "$@4", "$@5", "$@6", "$@7", "$@8", "$@9", "$@10", "$@11", "$@12", "$@13",
  "$@14", "$@15", "$@16", "$@17", "$@18", "$@19", "$@20", "$@21", "$@22",
  "$@23", "$@24", "$@25", "$@26", "$@27", "$@28", "$@29", "$@30", "$@31",
  "$@32", "$@33", "$@34", "$@35", "$@36", "$@37", "$@38", "$@39", "$@40",
  "$@41", "$@42", "$@43", "$@44", "$@45", "$@46", "$@47", "$@48", "$@49",
  "$@50", "$@51", "$@52", "$@53", "$@54", "$@55", "$@56", "$@57", "$@58",
  "$@59", "$@60", "op_subadd", "op_dp20_rm_l", "op_dp20_rm", "op_dp20_i",
  "op_dp20_rim", "op_dp20_rim_l", "op_dp20_rr", "op_dp20_r", "op_dp20_ri",
  "$@61", "op_xchg", "op_shift_rot", "op_shift", "float3_op", "float2_op",
  "$@62", "float2_op_ni", "$@63", "$@64", "mvfa_op", "$@65", "op_xor",
  "op_bfield", "$@66", "op_save_rstr", "double2_op", "double3_op", "disp",
  "flag", "$@67", "memex", "bwl", "bw", "opt_l", "opt_b", YY_NULLPTR
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
     385,   386,   387,   388,   389,   390,   391,   392,   393,   394,
     395,   396,   397,   398,   399,   400,   401,   402,   403,   404,
     405,   406,   407,   408,   409,   410,   411,   412,   413,   414,
     415,   416,   417,   418,    35,    44,    91,    93,    45,    43
};
# endif

#define YYPACT_NINF -728

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-728)))

#define YYTABLE_NINF -324

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     323,  -728,  -728,  -728,  -136,  -129,     2,   116,  -728,  -728,
    -120,    29,   127,  -728,    31,   136,    33,  -728,    12,  -728,
    -728,  -728,    45,  -728,  -728,  -728,   170,  -728,  -728,   183,
     193,  -728,  -728,  -728,  -728,  -728,  -728,    66,    77,  -118,
      79,   -98,  -728,  -728,  -728,  -728,  -728,  -728,   110,  -728,
    -728,   -30,   143,  -728,   155,   158,   191,   210,   221,  -728,
    -728,    41,   244,    85,    34,   249,   250,   251,    99,   252,
     253,   254,   255,  -728,   256,   257,   259,   258,  -728,   262,
     263,   264,    35,   266,   112,  -728,  -728,  -728,   113,   268,
     269,   270,   162,   273,   272,   115,   118,   119,   120,  -728,
    -728,   162,   277,   282,   124,   128,  -728,  -728,  -728,  -728,
    -728,   129,   286,  -728,  -728,   130,   227,  -728,  -728,  -728,
    -728,  -728,  -728,  -728,  -728,   162,  -728,  -728,   131,   162,
     162,  -728,   287,  -728,  -728,  -728,  -728,   261,   288,    19,
     285,    64,   289,    64,   132,   290,  -728,   291,   292,   294,
    -728,  -728,   295,   133,   296,  -728,   297,   298,   299,  -728,
     300,   309,   134,   303,  -728,   304,   305,   314,   153,   308,
    -728,   317,   157,  -728,   312,   160,   320,   321,   159,   321,
      22,    22,    18,     6,   321,   320,   319,   324,   322,   326,
     320,   320,   321,   320,   320,   320,   165,   169,   172,    91,
     173,   172,    91,    26,    37,    37,    26,    26,   334,   174,
     334,   334,   329,   176,    91,  -728,  -728,   177,   178,   179,
      22,    22,   216,   276,   283,   380,     5,   311,   415,  -728,
    -728,     7,   327,   330,   332,   476,    64,   333,   335,   336,
    -728,  -728,  -728,  -728,  -728,  -728,  -728,   337,   338,   339,
     340,   341,   342,   478,   343,   480,   288,   288,   483,    64,
    -728,  -728,   344,  -728,  -728,  -728,    92,  -728,   346,   498,
     499,   500,   504,   513,   513,  -728,  -728,  -728,   506,   513,
     507,   513,   334,    38,   508,  -728,    38,   509,    93,   518,
     511,  -728,    39,    39,    39,  -728,   172,   172,   512,    64,
    -728,  -728,    22,   359,    91,    91,    28,  -728,   360,  -728,
     361,   516,  -728,  -728,  -728,   362,   364,   365,  -728,   366,
     368,  -728,    94,   369,  -728,  -728,  -728,  -728,   367,  -728,
     370,    95,   371,  -728,  -728,  -728,  -728,  -728,    96,   372,
    -728,  -728,  -728,    97,   373,  -728,   536,   375,   538,   377,
    -728,   378,  -728,   537,  -728,   381,  -728,  -728,  -728,   379,
    -728,   382,   383,   384,   539,   386,   387,   542,   551,   389,
    -728,  -728,   388,   390,   391,   392,  -728,  -728,  -728,  -728,
    -728,  -728,   554,   558,  -728,   397,  -728,   398,   560,  -728,
    -728,   400,   555,  -728,   401,  -728,   404,  -728,   566,   511,
    -728,  -728,  -728,  -728,   563,  -728,  -728,  -728,   564,  -728,
     569,   570,   571,  -728,  -728,   565,   567,   568,   410,   412,
     414,    -1,   416,   417,   418,   419,     0,   578,   583,   584,
     423,  -728,   586,   587,   588,  -728,   428,  -728,  -728,  -728,
     590,   591,   589,   592,   593,   595,   431,   594,  -728,  -728,
    -728,   432,  -728,   598,  -728,   436,   600,   440,   441,   442,
     443,   444,  -728,  -728,   445,  -728,   446,  -728,  -728,  -728,
     601,  -728,   448,  -728,   449,  -728,  -728,   450,   604,  -728,
    -728,  -728,  -728,  -728,  -728,   614,  -728,   453,  -728,  -728,
     612,  -728,  -728,   455,  -728,  -728,   618,   619,   458,   621,
     622,   623,   624,   625,  -728,   463,    98,   620,   107,  -728,
     464,   108,  -728,   466,   109,  -728,   467,   111,  -728,   631,
     468,   629,   630,  -728,   635,   636,   231,   637,   638,   477,
     642,    40,   479,   484,   640,   643,   639,   644,   645,   490,
     491,   654,   655,   494,   657,   496,   659,   634,   501,   497,
    -728,  -728,   502,   503,   505,   510,   514,   515,   661,     8,
     662,    62,   666,   668,   517,   669,   670,    63,   671,   519,
     520,   521,   673,   522,   523,   524,   667,  -728,  -728,  -728,
    -728,  -728,  -728,   672,  -728,   678,  -728,   680,  -728,   684,
     685,   686,   687,   691,   692,   693,  -728,   695,   696,   697,
     540,   541,  -728,   698,  -728,   699,  -728,  -728,   700,   543,
     544,   546,   545,  -728,   701,  -728,   547,   549,  -728,   550,
     704,  -728,   552,   705,  -728,   553,   712,  -728,   556,  -728,
     140,  -728,   559,  -728,   561,  -728,  -728,  -728,  -728,   237,
    -728,  -728,   714,   557,   713,   562,   572,  -728,  -728,  -728,
    -728,   719,   720,  -728,   573,   723,   576,   717,   575,   579,
     727,   728,   729,   730,   731,    -4,    32,     9,  -728,  -728,
     577,     1,   581,   735,   580,   582,   585,   596,   743,  -728,
     597,   747,   602,   599,   603,   745,   748,   749,  -728,   750,
     751,   752,   606,  -728,  -728,   605,  -728,  -728,  -728,  -728,
    -728,  -728,  -728,   607,  -728,   609,   756,   757,  -728,   608,
    -728,   245,   758,   759,   192,   610,   762,   615,   765,   611,
     766,   613,   771,   616,   778,  -728,  -728,  -728,   617,  -728,
     626,   726,   200,  -728,  -728,   627,   781,  -728,   746,   628,
    -728,  -728,   206,  -728,   782,  -728,   245,   783,  -728,   632,
    -728,  -728,  -728,   784,   641,   785,   646,  -728,   786,   647,
     787,    68,   789,   633,   648,   125,   649,   651,  -728,  -728,
     652,   653,   792,   656,   658,  -728,  -728,  -728,  -728,  -728,
    -728,   790,  -728,   794,  -728,   660,  -728,   797,   663,  -728,
    -728,   664,   665,   791,   674,   793,   675,   791,   676,   791,
     677,   791,   679,   798,   799,  -728,   682,   683,  -728,   688,
    -728,   801,   689,   694,  -728,   702,  -728,   245,   690,   802,
     703,   806,   706,   807,   707,   815,  -728,   708,   709,   126,
     715,  -728,   711,   816,   819,   821,   716,  -728,   823,   824,
     718,  -728,   828,  -728,   829,   830,   831,  -728,  -728,   820,
     721,   791,  -728,   791,  -728,   822,  -728,   825,  -728,  -728,
     833,   835,  -728,  -728,   836,   842,   846,   722,  -728,   724,
    -728,   725,  -728,   732,  -728,   733,  -728,  -728,  -728,   736,
     847,   848,  -728,  -728,  -728,   849,  -728,  -728,   850,  -728,
    -728,  -728,  -728,  -728,   734,  -728,  -728,  -728,  -728,  -728,
    -728,  -728,  -728,  -728,   853,  -728,  -728,  -728,  -728,   855,
    -728,   737,  -728,  -728,   851,  -728,   738,  -728,   741,  -728,
     857,   742,   858,  -728
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
       0,     2,    97,    95,   210,   214,     0,     0,   235,   237,
       0,     0,   353,     3,     0,   353,     0,   339,   337,   243,
     257,     4,     0,   259,   107,   109,     0,   261,   245,     0,
       0,   247,   249,   263,   251,   253,   255,     0,     0,   121,
       0,   123,   143,   141,   147,   145,   139,   149,     0,   151,
     153,     0,     0,   127,     0,     0,     0,     0,     0,    99,
     101,   346,     0,     0,   350,     0,     0,     0,   212,     0,
       0,     0,   174,   229,   177,   180,     0,     0,   284,     0,
       0,     0,     0,     0,     0,    93,     6,   115,   216,     0,
       0,     0,   346,     0,     0,     0,     0,     0,     0,   196,
     194,   346,     0,     0,   190,   192,   155,   239,    76,    75,
       5,     0,     0,    78,   241,    91,   346,    67,   339,    43,
      45,    41,    69,    70,    68,   346,   119,   117,   208,   346,
     346,   111,     0,   129,    77,   125,   113,     0,     0,   337,
       0,   337,     0,   337,     0,     0,    18,     0,     0,     0,
     331,   331,     0,     0,     0,     7,     0,     0,     0,   354,
       0,     0,     0,     0,    10,     0,     0,     0,     0,     0,
      62,     0,     0,   338,     0,     0,     0,     0,     0,     0,
     337,   337,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   337,
       0,     0,   337,   337,   325,   325,   337,   337,   325,     0,
     325,   325,     0,     0,   337,    63,    64,     0,     0,     0,
     337,   337,   347,   348,   349,     0,     0,     0,     0,   351,
     352,     0,     0,     0,     0,     0,   337,     0,     0,     0,
     173,   327,   327,   176,   327,   179,   327,     0,     0,     0,
     169,   171,     0,     0,     0,     0,     0,     0,     0,   337,
      58,    60,     0,   347,   348,   349,   337,    59,     0,     0,
       0,     0,     0,     0,     0,    74,    56,    55,     0,     0,
       0,     0,   325,     0,     0,    54,     0,     0,   337,   349,
     337,    61,     0,     0,     0,    73,   306,   306,     0,   337,
      71,    72,   337,     0,   337,   337,   337,     1,   304,    98,
       0,     0,   301,   302,    96,     0,     0,     0,   211,     0,
       0,   215,   337,     0,    12,    13,    17,   236,     0,   238,
       0,   337,     0,     9,    14,    15,     8,    65,   337,     0,
      16,    11,    66,   337,     0,   340,     0,     0,     0,     0,
     244,     0,   258,     0,   260,     0,   299,   300,   108,     0,
     110,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     262,   246,     0,     0,     0,     0,   248,   250,   264,   252,
     254,   256,     0,     0,   104,     0,   122,     0,     0,   106,
     124,     0,     0,   144,     0,   142,     0,   322,     0,   337,
     148,   146,   140,   150,     0,   152,   154,    50,     0,   128,
       0,     0,     0,   100,   102,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   213,     0,     0,     0,   175,     0,   230,   178,   181,
       0,     0,     0,     0,     0,     0,     0,     0,    79,    94,
     116,     0,   217,     0,    57,     0,     0,     0,   182,     0,
       0,     0,   197,   195,     0,   191,     0,   193,   156,   334,
       0,   240,    40,   242,     0,    92,   157,     0,     0,   315,
      44,    46,    42,   308,   120,     0,   118,     0,   209,   112,
       0,   130,   126,     0,   329,   114,     0,     0,     0,     0,
       0,     0,     0,     0,   132,     0,   337,     0,   337,   134,
       0,   337,   131,     0,   337,   133,     0,   337,    26,     0,
       0,     0,     0,   265,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     286,   287,   165,     0,   167,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   161,     0,   163,     0,   199,   283,   231,
     170,   172,   198,     0,   285,     0,    48,     0,    47,     0,
       0,     0,     0,     0,     0,     0,   333,     0,     0,     0,
       0,     0,   307,     0,   288,     0,   303,   293,     0,     0,
      34,   289,     0,    36,     0,    52,     0,     0,   203,     0,
       0,   204,     0,     0,    51,     0,     0,    53,     0,    33,
     343,   335,     0,   295,     0,   267,   268,   269,   270,     0,
     266,   271,     0,     0,     0,     0,     0,   280,   279,   282,
     281,     0,     0,   309,     0,     0,   317,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    39,    85,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    29,
       0,     0,     0,     0,     0,     0,     0,     0,    35,     0,
       0,     0,     0,   202,    37,     0,   232,   183,   233,   234,
     312,   200,   201,     0,   218,     0,     0,     0,    32,   295,
     298,   353,     0,     0,   343,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   341,   342,   344,     0,   345,
       0,     0,   343,   277,   278,     0,     0,   276,     0,     0,
     221,   222,   343,   223,     0,   316,   353,     0,   324,     0,
     166,   224,   168,     0,     0,     0,     0,    38,     0,     0,
       0,     0,     0,     0,     0,   337,     0,     0,   219,   220,
       0,     0,     0,     0,     0,   225,   226,   227,   162,   228,
     164,     0,    90,     0,   158,   313,   305,     0,     0,    49,
     292,     0,     0,   355,     0,     0,     0,   355,     0,   355,
       0,   355,     0,     0,     0,   336,     0,     0,   272,     0,
     274,     0,     0,     0,   319,     0,   321,   353,     0,     0,
       0,     0,     0,     0,     0,     0,    82,     0,     0,   337,
       0,    86,     0,     0,     0,     0,     0,    30,     0,     0,
       0,    25,     0,   330,     0,     0,     0,   356,   136,     0,
       0,   355,   138,   355,   135,     0,   137,     0,    27,    28,
       0,     0,   273,   275,     0,     0,     0,     0,    19,     0,
      20,     0,    21,     0,    80,     0,   184,   185,    81,     0,
       0,     0,   186,   187,    31,     0,   188,   189,     0,   314,
     294,   290,   291,    88,     0,   159,   160,    87,    89,   296,
     297,   310,   311,   318,     0,    22,    23,    24,   205,     0,
     206,     0,   207,   328,     0,   326,     0,    83,     0,    84,
       0,     0,     0,   332
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -728,  -728,  -728,  -728,  -728,  -728,  -728,  -728,  -728,  -728,
    -728,  -728,  -728,  -728,  -728,  -728,  -728,  -728,  -728,  -728,
    -728,  -728,  -728,  -728,  -728,  -728,  -728,  -728,  -728,  -728,
    -728,  -728,  -728,  -728,  -728,  -728,  -728,  -728,  -728,  -728,
    -728,  -728,  -728,  -728,  -728,  -728,  -728,  -728,  -728,  -728,
    -728,  -728,  -728,  -728,  -728,  -728,  -728,  -728,  -728,  -728,
    -728,  -728,  -119,   650,  -728,  -133,  -167,  -728,  -141,  -728,
     437,  -728,  -154,  -196,  -145,    43,   710,  -728,  -149,  -728,
    -728,    -8,  -728,  -728,   739,  -728,   681,  -104,  -108,   -18,
     753,  -728,  -669,   -37,  -728,   -14,  -727
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,   137,   294,   292,   293,   288,   256,   139,   138,   220,
     221,   198,   201,   180,   181,   302,   306,   257,   297,   296,
     199,   202,   305,   214,   304,   207,   204,   203,   206,   205,
     208,   210,   211,   282,   241,   244,   246,   279,   281,   274,
     273,   299,   141,   236,   143,   259,   242,   150,   151,   283,
     286,   176,   185,   190,   191,   193,   194,   195,   177,   179,
     184,   192,   318,   312,   356,   357,   358,   314,   309,   602,
     484,   485,   386,   479,   480,   393,   395,   396,   397,   398,
     399,   435,   436,   495,   327,   328,   471,   350,   352,   359,
     170,   171,   730,   226,   231,   161,   848
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
     175,   167,   561,   567,   761,   144,   313,   173,   419,   365,
     424,   669,   757,   366,   360,   172,   420,   173,   425,   670,
     173,   361,   310,   173,   321,   355,   362,   363,   140,   391,
     173,   493,   153,   173,   162,   142,   168,   173,   252,   173,
    -323,   469,   461,   173,   152,   792,  -103,   641,   390,   229,
     230,   173,   178,   413,   414,   266,   222,   223,   224,   403,
     409,   405,   406,   807,   275,   384,  -105,   317,   389,   196,
     852,   354,   854,   813,   856,   173,   370,   462,   463,   290,
     197,   371,   200,   465,   378,   467,   376,   377,   295,   379,
     380,   381,   300,   301,   385,   454,   310,   504,   509,   512,
     515,   615,   173,   173,   173,   173,   173,   173,   173,   173,
     618,   621,   624,   209,   627,   449,   450,   431,   173,   173,
     173,   315,   173,   319,   895,   319,   896,   146,   831,   878,
     147,   148,   149,   468,   212,   489,   173,   173,   155,   494,
     452,   156,   157,   158,   159,   160,   213,   164,   481,   482,
     491,   492,   165,   159,   166,   725,   726,   727,   215,   728,
     729,   216,   753,   483,   483,   369,   145,   562,   568,   762,
     367,   421,   368,   426,   671,   758,   174,   263,   264,   265,
     488,   387,   364,   311,   387,   394,   311,   182,   394,   394,
     392,   183,   311,   154,   217,   163,   387,   169,   755,   253,
     186,  -320,   470,   478,   187,   225,   642,   725,   726,   727,
     188,   791,   729,   218,   189,   725,   726,   727,   319,   806,
     729,   725,   726,   727,   219,   812,   729,   673,   681,   674,
     682,   675,   683,   825,   437,   826,   438,   827,   439,   635,
     636,   319,   263,   264,   289,   733,   734,   227,   455,   401,
     402,   228,   232,   233,   234,   237,   238,   239,   240,   243,
     245,   307,   159,   235,   247,   249,   250,   251,   248,   254,
     315,   260,   477,   262,   261,   268,   255,   258,   267,   269,
     276,   319,   270,   271,   272,   277,   387,   387,   278,   285,
     303,   308,   280,   284,   287,   298,   316,   322,   331,   338,
     320,   323,   324,   325,   505,   326,   330,   332,   333,   334,
     335,   336,   337,   510,   339,   340,   341,   342,   343,   344,
     513,   345,   346,   347,   353,   516,   348,   349,   351,   372,
     382,   373,   374,   375,   383,     1,   311,  -323,   388,   404,
     407,   408,   410,   411,   412,     2,     3,     4,     5,     6,
       7,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
     415,   549,    37,    38,    39,    40,    41,    42,    43,    44,
      45,   418,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,   423,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
     416,    93,    94,    95,    96,    97,    98,   417,    99,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   109,   110,
     111,   112,   113,   114,   115,   116,   117,   118,   119,   120,
     121,   122,   123,   124,   125,   126,   422,   127,   128,   129,
     130,   131,   132,   133,   134,   135,   136,   430,   616,   446,
     619,   448,   427,   622,   451,   428,   625,   429,   432,   628,
     433,   434,   440,   441,   442,   443,   444,   445,   447,   457,
     458,   459,   453,   643,   456,   460,   461,   464,   466,   472,
     474,   476,   173,   487,   490,   496,   497,   498,   499,   500,
     501,   507,   502,   503,   506,   508,   511,   514,   517,   518,
     519,   520,   521,   522,   523,   525,   524,   526,   527,   528,
     529,   530,   531,   532,   533,   534,   535,   539,   536,   537,
     538,   540,   541,   543,   542,   544,   545,   546,   547,   548,
     550,   551,   552,   553,   554,   558,   555,   559,   556,   557,
     560,   569,   563,   565,   564,   566,   570,   571,   572,   573,
     574,   575,   576,   577,   578,   579,   583,   585,   580,   581,
     582,   586,   587,   588,   584,   589,   590,   591,   592,   593,
     594,   595,   596,   597,   598,   600,   599,   601,   603,   604,
     605,   606,   607,   608,   609,   610,   611,   612,   613,   614,
     620,   617,   623,   626,   629,   630,   631,   632,   633,   634,
     637,   638,   639,   646,   644,   659,   648,   754,   756,   759,
     640,   645,   650,   647,   649,   651,   652,   653,   654,   655,
     656,   657,   658,   661,   668,   672,   660,   662,   663,   676,
     664,   677,   679,   680,   684,   665,   688,   693,   692,   666,
     667,   694,   678,   695,   685,   686,   687,   689,   690,   691,
     696,   697,   698,   699,   700,   701,   702,   788,   703,   704,
     705,   708,   709,   710,   715,   706,   707,   719,   721,   712,
     711,   713,   714,   716,   717,   723,   718,   735,   720,   722,
     745,   737,   724,   736,   731,   740,   741,   738,   732,   743,
     748,   749,   815,   805,   486,   750,   751,   752,   764,   739,
     742,   744,   746,   760,   747,   765,   769,   832,   763,   766,
     771,   775,   767,   810,   776,   777,   778,   779,   780,   785,
     786,   789,   790,   768,   770,   794,   773,   772,   796,   798,
     774,   781,   782,   787,   800,   783,   784,   793,   797,   795,
     799,   802,   803,   801,   809,   814,   816,   818,   820,   822,
     824,   804,   828,   811,   808,   837,   840,   841,   829,   817,
     843,   858,   859,   867,   850,   869,   847,   819,   863,   871,
     873,   879,   821,   823,   833,   830,   834,   835,   875,   882,
     836,   838,   883,   839,   884,   842,   886,   887,   844,   845,
     846,   889,   890,   891,   892,   893,   899,   897,   900,   901,
     898,   849,   851,   853,   855,   902,   857,   860,   861,   903,
     910,   911,   912,   913,   864,   862,   915,   868,   916,   865,
     921,   923,   918,     0,     0,     0,     0,   866,     0,     0,
     870,   291,     0,   872,   874,   876,   877,   881,     0,     0,
     880,   885,     0,   888,     0,     0,   894,   904,     0,     0,
     329,   905,   906,     0,     0,     0,     0,     0,   914,   907,
     908,     0,   909,     0,   917,   919,   920,   922,     0,     0,
       0,     0,     0,     0,     0,   400,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   475,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   473
};

static const yytype_int16 yycheck[] =
{
      18,    15,     3,     3,     3,     3,   139,    11,     3,     3,
       3,     3,     3,     7,   181,     3,    11,    11,    11,    11,
      11,     3,     3,    11,   143,     3,     8,     9,   164,     3,
      11,     3,     3,    11,     3,   164,     3,    11,     3,    11,
       3,     3,     3,    11,   164,   714,   164,     7,   202,    15,
      16,    11,     7,   220,   221,    92,    15,    16,    17,   208,
     214,   210,   211,   732,   101,   198,   164,     3,   201,     3,
     797,   179,   799,   742,   801,    11,   184,   273,   274,   116,
       3,   185,     3,   279,   192,   281,   190,   191,   125,   193,
     194,   195,   129,   130,     3,     3,     3,     3,     3,     3,
       3,     3,    11,    11,    11,    11,    11,    11,    11,    11,
       3,     3,     3,     3,     3,   256,   257,   236,    11,    11,
      11,   139,    11,   141,   851,   143,   853,    11,     3,     3,
      14,    15,    16,   282,   164,   302,    11,    11,    11,   306,
     259,    14,    15,    16,    17,    18,     3,    11,   293,   294,
     304,   305,    16,    17,    18,    15,    16,    17,     3,    19,
      20,     3,   166,   296,   297,   183,   164,   168,   168,   168,
     164,   166,   166,   166,   166,   166,   164,    15,    16,    17,
     299,   199,   164,   164,   202,   203,   164,    17,   206,   207,
     164,    21,   164,   164,     3,   164,   214,   164,   166,   164,
      17,   164,   164,   164,    21,   164,   166,    15,    16,    17,
      17,    19,    20,     3,    21,    15,    16,    17,   236,    19,
      20,    15,    16,    17,     3,    19,    20,   165,   165,   167,
     167,   169,   169,   165,   242,   167,   244,   169,   246,     8,
       9,   259,    15,    16,    17,     8,     9,     3,   266,   206,
     207,   166,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     0,    17,   164,     5,     3,     3,     3,    10,     3,
     288,     3,   290,     3,     5,     3,   164,   164,     5,   164,
       3,   299,   164,   164,   164,     3,   304,   305,   164,     3,
       3,     3,   164,   164,   164,   164,    11,   165,   165,   165,
      11,    11,    11,    11,   322,    11,    11,    11,    11,    11,
      11,    11,     3,   331,    11,    11,    11,     3,   165,    11,
     338,     4,   165,    11,   165,   343,   166,     7,     7,    10,
     165,     7,    10,     7,   165,    12,   164,     3,   165,   165,
      11,   165,   165,   165,   165,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
     164,   399,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    11,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,     3,    96,
      97,    98,    99,   100,   101,   102,   103,   104,   105,   106,
     107,   108,   109,   110,   111,   112,   113,   114,   115,   116,
     164,   118,   119,   120,   121,   122,   123,   164,   125,   126,
     127,   128,   129,   130,   131,   132,   133,   134,   135,   136,
     137,   138,   139,   140,   141,   142,   143,   144,   145,   146,
     147,   148,   149,   150,   151,   152,   165,   154,   155,   156,
     157,   158,   159,   160,   161,   162,   163,    11,   506,    11,
     508,    11,   165,   511,    11,   165,   514,   165,   165,   517,
     165,   165,   165,   165,   165,   165,   165,   165,   165,    11,
      11,    11,   168,   531,   168,    11,     3,    11,    11,    11,
      11,     3,    11,    11,   165,   165,   165,    11,   166,   165,
     165,   164,   166,   165,   165,   165,   165,   165,   165,     3,
     165,     3,   165,   165,     7,   166,   165,   165,   165,   165,
      11,   165,   165,    11,     3,   166,   168,     3,   168,   168,
     168,     3,   165,     3,   166,   165,    11,   166,   164,     3,
       7,     7,     3,     3,     3,   165,    11,   165,    11,    11,
     166,     3,   166,   165,   167,   166,     3,     3,   165,     3,
       3,     3,   164,     3,     3,     6,   165,   165,     6,     6,
       5,     3,   166,     3,    10,   165,   165,   165,   165,   165,
     165,   165,    11,   165,   165,    11,   166,     3,   165,     7,
     165,     3,     3,   165,     3,     3,     3,     3,     3,   166,
     166,    11,   166,   166,     3,   167,     7,     7,     3,     3,
       3,     3,   165,     3,   165,    11,     7,   665,   666,   667,
       8,   167,     7,    10,    10,   165,   165,     3,     3,   165,
       3,   165,     3,   166,     3,     3,   165,   165,   165,     3,
     165,     3,     3,     3,     3,   165,     3,     5,    11,   165,
     165,     3,   165,     3,   165,   165,   165,   165,   165,   165,
       6,     6,     6,     6,     3,     3,     3,   711,     3,     3,
       3,     3,     3,     3,     3,   165,   165,     3,     3,   165,
     167,   165,   167,   166,   165,     3,   166,     3,   166,   166,
       3,     8,   166,   166,   165,     6,     6,   165,   167,     6,
       3,     3,   746,     7,   297,     6,     6,     6,     3,   167,
     167,   165,   167,   166,   165,   165,     3,   765,   167,   167,
       3,     6,   167,     7,     6,     6,     6,     6,     6,     3,
       3,     3,     3,   167,   167,     3,   167,   165,     3,     3,
     167,   165,   167,   165,     3,   168,   167,   167,   167,   164,
     167,     3,   165,   167,     3,     3,     3,     3,     3,     3,
       3,   165,     3,   165,   167,     3,     6,     3,   165,   167,
       3,     3,     3,   817,    11,     3,    15,   166,     7,     3,
       3,   829,   166,   166,   165,   167,   165,   165,     3,     3,
     167,   165,     3,   165,     3,   165,     3,     3,   165,   165,
     165,     3,     3,     3,     3,    15,     3,    15,     3,     3,
      15,   167,   167,   167,   167,     3,   167,   165,   165,     3,
       3,     3,     3,     3,   165,   167,     3,   167,     3,   165,
       3,     3,    11,    -1,    -1,    -1,    -1,   165,    -1,    -1,
     167,   118,    -1,   167,   167,   167,   167,   166,    -1,    -1,
     165,   165,    -1,   165,    -1,    -1,   165,   165,    -1,    -1,
     151,   167,   167,    -1,    -1,    -1,    -1,    -1,   164,   167,
     167,    -1,   166,    -1,   167,   167,   165,   165,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   205,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   288,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   286
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint16 yystos[] =
{
       0,    12,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    96,    97,    98,    99,   100,   101,   102,   103,
     104,   105,   106,   107,   108,   109,   110,   111,   112,   113,
     114,   115,   116,   118,   119,   120,   121,   122,   123,   125,
     126,   127,   128,   129,   130,   131,   132,   133,   134,   135,
     136,   137,   138,   139,   140,   141,   142,   143,   144,   145,
     146,   147,   148,   149,   150,   151,   152,   154,   155,   156,
     157,   158,   159,   160,   161,   162,   163,   171,   178,   177,
     164,   212,   164,   214,     3,   164,    11,    14,    15,    16,
     217,   218,   164,     3,   164,    11,    14,    15,    16,    17,
      18,   265,     3,   164,    11,    16,    18,   265,     3,   164,
     260,   261,     3,    11,   164,   259,   221,   228,     7,   229,
     183,   184,    17,    21,   230,   222,    17,    21,    17,    21,
     223,   224,   231,   225,   226,   227,     3,     3,   181,   190,
       3,   182,   191,   197,   196,   199,   198,   195,   200,     3,
     201,   202,   164,     3,   193,     3,     3,     3,     3,     3,
     179,   180,    15,    16,    17,   164,   263,     3,   166,    15,
      16,   264,     3,     3,     3,   164,   213,     3,     3,     3,
       3,   204,   216,     3,   205,     3,   206,     5,    10,     3,
       3,     3,     3,   164,     3,   164,   176,   187,   164,   215,
       3,     5,     3,    15,    16,    17,   263,     5,     3,   164,
     164,   164,   164,   210,   209,   263,     3,     3,   164,   207,
     164,   208,   203,   219,   164,     3,   220,   164,   175,    17,
     263,   260,   173,   174,   172,   263,   189,   188,   164,   211,
     263,   263,   185,     3,   194,   192,   186,     0,     3,   238,
       3,   164,   233,   235,   237,   259,    11,     3,   232,   259,
      11,   232,   165,    11,    11,    11,    11,   254,   255,   254,
      11,   165,    11,    11,    11,    11,    11,     3,   165,    11,
      11,    11,     3,   165,    11,     4,   165,    11,   166,     7,
     257,     7,   258,   165,   258,     3,   234,   235,   236,   259,
     236,     3,     8,     9,   164,     3,     7,   164,   166,   259,
     258,   257,    10,     7,    10,     7,   257,   257,   258,   257,
     257,   257,   165,   165,   235,     3,   242,   259,   165,   235,
     242,     3,   164,   245,   259,   246,   247,   248,   249,   250,
     246,   245,   245,   248,   165,   248,   248,    11,   165,   242,
     165,   165,   165,   236,   236,   164,   164,   164,    11,     3,
      11,   166,   165,     3,     3,    11,   166,   165,   165,   165,
      11,   232,   165,   165,   165,   251,   252,   251,   251,   251,
     165,   165,   165,   165,   165,   165,    11,   165,    11,   238,
     238,    11,   232,   168,     3,   259,   168,    11,    11,    11,
      11,     3,   243,   243,    11,   243,    11,   243,   248,     3,
     164,   256,    11,   256,    11,   233,     3,   259,   164,   243,
     244,   244,   244,   235,   240,   241,   240,    11,   232,   236,
     165,   242,   242,     3,   236,   253,   165,   165,    11,   166,
     165,   165,   166,   165,     3,   259,   165,   164,   165,     3,
     259,   165,     3,   259,   165,     3,   259,   165,     3,   165,
       3,   165,   165,     7,   165,   166,   165,   165,   165,    11,
     165,   165,    11,     3,   166,   168,   168,   168,   168,     3,
       3,   165,   166,     3,   165,    11,   166,   164,     3,   259,
       7,     7,     3,     3,     3,    11,    11,    11,   165,   165,
     166,     3,   168,   166,   167,   165,   166,     3,   168,     3,
       3,     3,   165,     3,     3,     3,   164,     3,     3,     6,
       6,     6,     5,   165,    10,   165,     3,   166,     3,   165,
     165,   165,   165,   165,   165,   165,    11,   165,   165,   166,
      11,     3,   239,   165,     7,   165,     3,     3,   165,     3,
       3,     3,     3,     3,   166,     3,   259,    11,     3,   259,
     166,     3,   259,   166,     3,   259,   166,     3,   259,     3,
     167,     7,     7,     3,     3,     8,     9,     3,     3,   165,
       8,     7,   166,   259,   165,   167,     3,    10,     7,    10,
       7,   165,   165,     3,     3,   165,     3,   165,     3,    11,
     165,   166,   165,   165,   165,   165,   165,   165,     3,     3,
      11,   166,     3,   165,   167,   169,     3,     3,   165,     3,
       3,   165,   167,   169,     3,   165,   165,   165,     3,   165,
     165,   165,    11,     5,     3,     3,     6,     6,     6,     6,
       3,     3,     3,     3,     3,     3,   165,   165,     3,     3,
       3,   167,   165,   165,   167,     3,   166,   165,   166,     3,
     166,     3,   166,     3,   166,    15,    16,    17,    19,    20,
     262,   165,   167,     8,     9,     3,   166,     8,   165,   167,
       6,     6,   167,     6,   165,     3,   167,   165,     3,     3,
       6,     6,     6,   166,   259,   166,   259,     3,   166,   259,
     166,     3,   168,   167,     3,   165,   167,   167,   167,     3,
     167,     3,   165,   167,   167,     6,     6,     6,     6,     6,
       6,   165,   167,   168,   167,     3,     3,   165,   265,     3,
       3,    19,   262,   167,     3,   164,     3,   167,     3,   167,
       3,   167,     3,   165,   165,     7,    19,   262,   167,     3,
       7,   165,    19,   262,     3,   265,     3,   167,     3,   166,
       3,   166,     3,   166,     3,   165,   167,   169,     3,   165,
     167,     3,   259,   165,   165,   165,   167,     3,   165,   165,
       6,     3,   165,     3,   165,   165,   165,    15,   266,   167,
      11,   167,   266,   167,   266,   167,   266,   167,     3,     3,
     165,   165,   167,     7,   165,   165,   165,   265,   167,     3,
     167,     3,   167,     3,   167,     3,   167,   167,     3,   259,
     165,   166,     3,     3,     3,   165,     3,     3,   165,     3,
       3,     3,     3,    15,   165,   266,   266,    15,    15,     3,
       3,     3,     3,     3,   165,   167,   167,   167,   167,   166,
       3,     3,     3,     3,   164,     3,     3,   167,    11,   167,
     165,     3,   165,     3
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint16 yyr1[] =
{
       0,   170,   171,   171,   171,   171,   171,   171,   171,   171,
     171,   171,   171,   171,   171,   171,   171,   171,   171,   171,
     171,   171,   171,   171,   171,   171,   171,   171,   171,   171,
     171,   171,   171,   171,   171,   171,   171,   171,   171,   171,
     171,   172,   171,   173,   171,   174,   171,   171,   171,   171,
     171,   171,   171,   171,   171,   171,   171,   171,   171,   171,
     171,   171,   171,   171,   171,   171,   171,   171,   171,   171,
     171,   171,   171,   171,   171,   171,   171,   171,   171,   171,
     171,   171,   171,   171,   171,   171,   171,   171,   171,   171,
     171,   175,   171,   176,   171,   177,   171,   178,   171,   179,
     171,   180,   171,   181,   171,   182,   171,   183,   171,   184,
     171,   185,   171,   186,   171,   187,   171,   188,   171,   189,
     171,   190,   171,   191,   171,   192,   171,   193,   171,   194,
     171,   171,   171,   171,   171,   171,   171,   171,   171,   195,
     171,   196,   171,   197,   171,   198,   171,   199,   171,   200,
     171,   201,   171,   202,   171,   203,   171,   171,   171,   171,
     171,   171,   171,   171,   171,   171,   171,   171,   171,   171,
     171,   171,   171,   171,   204,   171,   171,   205,   171,   171,
     206,   171,   171,   171,   171,   171,   171,   171,   171,   171,
     207,   171,   208,   171,   209,   171,   210,   171,   171,   171,
     171,   171,   171,   171,   171,   171,   171,   171,   211,   171,
     212,   171,   213,   171,   214,   171,   215,   171,   171,   171,
     171,   171,   171,   171,   171,   171,   171,   171,   171,   216,
     171,   171,   171,   171,   171,   217,   171,   218,   171,   219,
     171,   220,   171,   221,   171,   222,   171,   223,   171,   224,
     171,   225,   171,   226,   171,   227,   171,   228,   171,   229,
     171,   230,   171,   231,   171,   171,   171,   171,   171,   171,
     171,   171,   171,   171,   171,   171,   171,   171,   171,   171,
     171,   171,   171,   171,   171,   171,   171,   171,   171,   232,
     232,   232,   232,   233,   233,   234,   234,   234,   235,   236,
     236,   237,   237,   238,   238,   239,   241,   240,   240,   242,
     242,   242,   243,   244,   244,   244,   245,   245,   245,   245,
     247,   246,   246,   249,   248,   250,   248,   252,   251,   253,
     253,   255,   254,   256,   256,   257,   258,   259,   259,   261,
     260,   262,   262,   262,   262,   262,   263,   263,   263,   263,
     264,   264,   264,   265,   265,   266,   266
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
       3,     4,     5,     5,     5,     0,     3,     0,     3,     0,
       3,     0,     3,     0,     3,     0,     3,     0,     3,     0,
       3,     0,     3,     0,     3,     0,     3,     0,     3,     0,
       3,     0,     3,     0,     3,     4,     5,     5,     5,     5,
       5,     5,     7,     8,     7,     8,     6,     6,     6,     5,
       5,     5,     5,     4,     1,     4,     4,     4,     4,     3,
       7,     7,     5,     3,     7,     3,     7,     7,     4,     1,
       1,     1,     1,     3,     1,     3,     0,     2,     1,     3,
       7,     7,     3,     4,     6,     1,     4,     3,     7,     5,
       0,     5,     1,     0,     4,     0,     8,     0,     7,     1,
       5,     0,    13,     2,     1,     3,     5,     0,     1,     0,
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
    default: /* Avoid compiler warnings. */
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
#line 185 "./config/rx-parse.y" /* yacc.c:1648  */
    { as_bad (_("Unknown opcode: %s"), rx_init_start); }
#line 2300 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 3:
#line 190 "./config/rx-parse.y" /* yacc.c:1648  */
    { B1 (0x00); }
#line 2306 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 4:
#line 193 "./config/rx-parse.y" /* yacc.c:1648  */
    { B1 (0x01); }
#line 2312 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 5:
#line 196 "./config/rx-parse.y" /* yacc.c:1648  */
    { B1 (0x02); }
#line 2318 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 6:
#line 199 "./config/rx-parse.y" /* yacc.c:1648  */
    { B1 (0x03); }
#line 2324 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 7:
#line 204 "./config/rx-parse.y" /* yacc.c:1648  */
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
#line 2342 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 8:
#line 219 "./config/rx-parse.y" /* yacc.c:1648  */
    { B1 (0x04); PC3 ((yyvsp[0].exp)); }
#line 2348 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 9:
#line 222 "./config/rx-parse.y" /* yacc.c:1648  */
    { B1 (0x08); rx_disp3 ((yyvsp[0].exp), 5); }
#line 2354 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 10:
#line 227 "./config/rx-parse.y" /* yacc.c:1648  */
    { if (rx_intop ((yyvsp[0].exp), 16, 16))
	      { B1 (0x39); PC2 ((yyvsp[0].exp)); }
	    else if (rx_intop ((yyvsp[0].exp), 24, 24))
	      { B1 (0x05); PC3 ((yyvsp[0].exp)); }
	    else
	      { rx_relax (RX_RELAX_BRANCH, 0);
		rx_linkrelax_branch ();
		B1 (0x39); PC2 ((yyvsp[0].exp)); } }
#line 2367 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 11:
#line 236 "./config/rx-parse.y" /* yacc.c:1648  */
    { B1 (0x05), PC3 ((yyvsp[0].exp)); }
#line 2373 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 12:
#line 241 "./config/rx-parse.y" /* yacc.c:1648  */
    { if ((yyvsp[-2].regno) == COND_EQ || (yyvsp[-2].regno) == COND_NE)
	      { B1 ((yyvsp[-2].regno) == COND_EQ ? 0x10 : 0x18); rx_disp3 ((yyvsp[0].exp), 5); }
	    else
	      as_bad (_("Only BEQ and BNE may have .S")); }
#line 2382 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 13:
#line 249 "./config/rx-parse.y" /* yacc.c:1648  */
    { B1 (0x20); F ((yyvsp[-2].regno), 4, 4); PC1 ((yyvsp[0].exp)); }
#line 2388 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 14:
#line 252 "./config/rx-parse.y" /* yacc.c:1648  */
    { B1 (0x2e), PC1 ((yyvsp[0].exp)); }
#line 2394 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 15:
#line 257 "./config/rx-parse.y" /* yacc.c:1648  */
    { B1 (0x38), PC2 ((yyvsp[0].exp)); }
#line 2400 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 16:
#line 259 "./config/rx-parse.y" /* yacc.c:1648  */
    { B1 (0x39), PC2 ((yyvsp[0].exp)); }
#line 2406 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 17:
#line 261 "./config/rx-parse.y" /* yacc.c:1648  */
    { if ((yyvsp[-2].regno) == COND_EQ || (yyvsp[-2].regno) == COND_NE)
	      { B1 ((yyvsp[-2].regno) == COND_EQ ? 0x3a : 0x3b); PC2 ((yyvsp[0].exp)); }
	    else
	      as_bad (_("Only BEQ and BNE may have .W")); }
#line 2415 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 18:
#line 266 "./config/rx-parse.y" /* yacc.c:1648  */
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
#line 2434 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 19:
#line 284 "./config/rx-parse.y" /* yacc.c:1648  */
    { B2 (0xf8, 0x04); F ((yyvsp[-1].regno), 8, 4); IMMB ((yyvsp[-4].exp), 12);}
#line 2440 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 20:
#line 287 "./config/rx-parse.y" /* yacc.c:1648  */
    { B2 (0xf8, 0x01); F ((yyvsp[-1].regno), 8, 4); IMMW ((yyvsp[-4].exp), 12);}
#line 2446 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 21:
#line 290 "./config/rx-parse.y" /* yacc.c:1648  */
    { B2 (0xf8, 0x02); F ((yyvsp[-1].regno), 8, 4); IMM ((yyvsp[-4].exp), 12);}
#line 2452 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 22:
#line 294 "./config/rx-parse.y" /* yacc.c:1648  */
    { if ((yyvsp[-1].regno) <= 7 && rx_uintop ((yyvsp[-5].exp), 8) && rx_disp5op0 (&(yyvsp[-3].exp), BSIZE))
	      { B2 (0x3c, 0); rx_field5s2 ((yyvsp[-3].exp)); F ((yyvsp[-1].regno), 9, 3); O1 ((yyvsp[-5].exp)); }
	    else
	      { B2 (0xf8, 0x04); F ((yyvsp[-1].regno), 8, 4); DSP ((yyvsp[-3].exp), 6, BSIZE); O1 ((yyvsp[-5].exp));
	      if ((yyvsp[-5].exp).X_op != O_constant && (yyvsp[-5].exp).X_op != O_big) rx_linkrelax_imm (12); } }
#line 2462 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 23:
#line 301 "./config/rx-parse.y" /* yacc.c:1648  */
    { if ((yyvsp[-1].regno) <= 7 && rx_uintop ((yyvsp[-5].exp), 8) && rx_disp5op0 (&(yyvsp[-3].exp), WSIZE))
	      { B2 (0x3d, 0); rx_field5s2 ((yyvsp[-3].exp)); F ((yyvsp[-1].regno), 9, 3); O1 ((yyvsp[-5].exp)); }
	    else
	      { B2 (0xf8, 0x01); F ((yyvsp[-1].regno), 8, 4); DSP ((yyvsp[-3].exp), 6, WSIZE); IMMW ((yyvsp[-5].exp), 12); } }
#line 2471 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 24:
#line 307 "./config/rx-parse.y" /* yacc.c:1648  */
    { if ((yyvsp[-1].regno) <= 7 && rx_uintop ((yyvsp[-5].exp), 8) && rx_disp5op0 (&(yyvsp[-3].exp), LSIZE))
	      { B2 (0x3e, 0); rx_field5s2 ((yyvsp[-3].exp)); F ((yyvsp[-1].regno), 9, 3); O1 ((yyvsp[-5].exp)); }
	    else
	      { B2 (0xf8, 0x02); F ((yyvsp[-1].regno), 8, 4); DSP ((yyvsp[-3].exp), 6, LSIZE); IMM ((yyvsp[-5].exp), 12); } }
#line 2480 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 25:
#line 315 "./config/rx-parse.y" /* yacc.c:1648  */
    { B2 (0x3f, 0); F ((yyvsp[-2].regno), 8, 4); F ((yyvsp[0].regno), 12, 4); rtsd_immediate ((yyvsp[-4].exp));
	    if ((yyvsp[-2].regno) == 0)
	      rx_error (_("RTSD cannot pop R0"));
	    if ((yyvsp[-2].regno) > (yyvsp[0].regno))
	      rx_error (_("RTSD first reg must be <= second reg")); }
#line 2490 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 26:
#line 324 "./config/rx-parse.y" /* yacc.c:1648  */
    { B2 (0x47, 0); F ((yyvsp[-2].regno), 8, 4); F ((yyvsp[0].regno), 12, 4); }
#line 2496 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 27:
#line 329 "./config/rx-parse.y" /* yacc.c:1648  */
    { B2 (0x44, 0); F ((yyvsp[-4].regno), 8, 4); F ((yyvsp[0].regno), 12, 4); DSP ((yyvsp[-6].exp), 6, BSIZE); }
#line 2502 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 28:
#line 332 "./config/rx-parse.y" /* yacc.c:1648  */
    { B3 (MEMEX, 0x04, 0); F ((yyvsp[-2].regno), 8, 2);  F ((yyvsp[-4].regno), 16, 4); F ((yyvsp[0].regno), 20, 4); DSP ((yyvsp[-6].exp), 14, sizemap[(yyvsp[-2].regno)]); }
#line 2508 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 29:
#line 337 "./config/rx-parse.y" /* yacc.c:1648  */
    { B2 (0x5b, 0x00); F ((yyvsp[-3].regno), 5, 1); F ((yyvsp[-2].regno), 8, 4); F ((yyvsp[0].regno), 12, 4); }
#line 2514 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 30:
#line 342 "./config/rx-parse.y" /* yacc.c:1648  */
    { B2 (0x58, 0x00); F ((yyvsp[-5].regno), 5, 1); F ((yyvsp[-3].regno), 8, 4); F ((yyvsp[0].regno), 12, 4); }
#line 2520 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 31:
#line 345 "./config/rx-parse.y" /* yacc.c:1648  */
    { if ((yyvsp[-3].regno) <= 7 && (yyvsp[0].regno) <= 7 && rx_disp5op (&(yyvsp[-5].exp), (yyvsp[-6].regno)))
	      { B2 (0xb0, 0); F ((yyvsp[-6].regno), 4, 1); F ((yyvsp[-3].regno), 9, 3); F ((yyvsp[0].regno), 13, 3); rx_field5s ((yyvsp[-5].exp)); }
	    else
	      { B2 (0x58, 0x00); F ((yyvsp[-6].regno), 5, 1); F ((yyvsp[-3].regno), 8, 4); F ((yyvsp[0].regno), 12, 4); DSP ((yyvsp[-5].exp), 6, (yyvsp[-6].regno)); } }
#line 2529 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 32:
#line 353 "./config/rx-parse.y" /* yacc.c:1648  */
    { if (rx_uintop ((yyvsp[-2].exp), 4))
	      { B2 (0x60, 0); FE ((yyvsp[-2].exp), 8, 4); F ((yyvsp[0].regno), 12, 4); }
	    else
	      /* This is really an add, but we negate the immediate.  */
	      { B2 (0x70, 0); F ((yyvsp[0].regno), 8, 4); F ((yyvsp[0].regno), 12, 4); NIMM ((yyvsp[-2].exp), 6); } }
#line 2539 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 33:
#line 360 "./config/rx-parse.y" /* yacc.c:1648  */
    { if (rx_uintop ((yyvsp[-2].exp), 4))
	      { B2 (0x61, 0); FE ((yyvsp[-2].exp), 8, 4); F ((yyvsp[0].regno), 12, 4); }
	    else if (rx_uintop ((yyvsp[-2].exp), 8))
	      { B2 (0x75, 0x50); F ((yyvsp[0].regno), 12, 4); UO1 ((yyvsp[-2].exp)); }
	    else
	      { B2 (0x74, 0x00); F ((yyvsp[0].regno), 12, 4); IMM ((yyvsp[-2].exp), 6); } }
#line 2550 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 34:
#line 368 "./config/rx-parse.y" /* yacc.c:1648  */
    { if (rx_uintop ((yyvsp[-2].exp), 4))
	      { B2 (0x62, 0); FE ((yyvsp[-2].exp), 8, 4); F ((yyvsp[0].regno), 12, 4); }
	    else
	      { B2 (0x70, 0); F ((yyvsp[0].regno), 8, 4); F ((yyvsp[0].regno), 12, 4); IMM ((yyvsp[-2].exp), 6); } }
#line 2559 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 35:
#line 374 "./config/rx-parse.y" /* yacc.c:1648  */
    { if (rx_uintop ((yyvsp[-2].exp), 4))
	      { B2 (0x63, 0); FE ((yyvsp[-2].exp), 8, 4); F ((yyvsp[0].regno), 12, 4); }
	    else
	      { B2 (0x74, 0x10); F ((yyvsp[0].regno), 12, 4); IMM ((yyvsp[-2].exp), 6); } }
#line 2568 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 36:
#line 380 "./config/rx-parse.y" /* yacc.c:1648  */
    { if (rx_uintop ((yyvsp[-2].exp), 4))
	      { B2 (0x64, 0); FE ((yyvsp[-2].exp), 8, 4); F ((yyvsp[0].regno), 12, 4); }
	    else
	      { B2 (0x74, 0x20); F ((yyvsp[0].regno), 12, 4); IMM ((yyvsp[-2].exp), 6); } }
#line 2577 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 37:
#line 386 "./config/rx-parse.y" /* yacc.c:1648  */
    { if (rx_uintop ((yyvsp[-2].exp), 4))
	      { B2 (0x65, 0); FE ((yyvsp[-2].exp), 8, 4); F ((yyvsp[0].regno), 12, 4); }
	    else
	      { B2 (0x74, 0x30); F ((yyvsp[0].regno), 12, 4); IMM ((yyvsp[-2].exp), 6); } }
#line 2586 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 38:
#line 392 "./config/rx-parse.y" /* yacc.c:1648  */
    { if (rx_uintop ((yyvsp[-2].exp), 4))
	      { B2 (0x66, 0); FE ((yyvsp[-2].exp), 8, 4); F ((yyvsp[0].regno), 12, 4); }
	    else if (rx_uintop ((yyvsp[-2].exp), 8))
	      { B2 (0x75, 0x40); F ((yyvsp[0].regno), 12, 4); UO1 ((yyvsp[-2].exp)); }
	    else
	      { B2 (0xfb, 0x02); F ((yyvsp[0].regno), 8, 4); IMM ((yyvsp[-2].exp), 12); } }
#line 2597 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 39:
#line 400 "./config/rx-parse.y" /* yacc.c:1648  */
    { if (rx_uintop ((yyvsp[-2].exp), 4))
	      { B2 (0x66, 0); FE ((yyvsp[-2].exp), 8, 4); F ((yyvsp[0].regno), 12, 4); }
	    else if (rx_uintop ((yyvsp[-2].exp), 8))
	      { B2 (0x75, 0x40); F ((yyvsp[0].regno), 12, 4); UO1 ((yyvsp[-2].exp)); }
	    else
	      { B2 (0xfb, 0x02); F ((yyvsp[0].regno), 8, 4); IMM ((yyvsp[-2].exp), 12); } }
#line 2608 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 40:
#line 410 "./config/rx-parse.y" /* yacc.c:1648  */
    { B1 (0x67); rtsd_immediate ((yyvsp[0].exp)); }
#line 2614 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 41:
#line 414 "./config/rx-parse.y" /* yacc.c:1648  */
    { sub_op = 0; }
#line 2620 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 43:
#line 415 "./config/rx-parse.y" /* yacc.c:1648  */
    { sub_op = 1; }
#line 2626 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 45:
#line 416 "./config/rx-parse.y" /* yacc.c:1648  */
    { sub_op = 2; }
#line 2632 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 47:
#line 421 "./config/rx-parse.y" /* yacc.c:1648  */
    {
	    if ((yyvsp[-2].regno) == (yyvsp[0].regno))
	      { B2 (0x7e, 0x80); F (LSIZE, 10, 2); F ((yyvsp[-2].regno), 12, 4); }
	    else
	     { B2 (0x6e, 0); F ((yyvsp[-2].regno), 8, 4); F ((yyvsp[0].regno), 12, 4); }
	    if ((yyvsp[-2].regno) == 0)
	      rx_error (_("PUSHM cannot push R0"));
	    if ((yyvsp[-2].regno) > (yyvsp[0].regno))
	      rx_error (_("PUSHM first reg must be <= second reg")); }
#line 2646 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 48:
#line 434 "./config/rx-parse.y" /* yacc.c:1648  */
    {
	    if ((yyvsp[-2].regno) == (yyvsp[0].regno))
	      { B2 (0x7e, 0xb0); F ((yyvsp[-2].regno), 12, 4); }
	    else
	      { B2 (0x6f, 0); F ((yyvsp[-2].regno), 8, 4); F ((yyvsp[0].regno), 12, 4); }
	    if ((yyvsp[-2].regno) == 0)
	      rx_error (_("POPM cannot pop R0"));
	    if ((yyvsp[-2].regno) > (yyvsp[0].regno))
	      rx_error (_("POPM first reg must be <= second reg")); }
#line 2660 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 49:
#line 447 "./config/rx-parse.y" /* yacc.c:1648  */
    { B2 (0x70, 0x00); F ((yyvsp[-2].regno), 8, 4); F ((yyvsp[0].regno), 12, 4); IMM ((yyvsp[-4].exp), 6); }
#line 2666 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 50:
#line 452 "./config/rx-parse.y" /* yacc.c:1648  */
    { B2(0x75, 0x60), UO1 ((yyvsp[0].exp)); }
#line 2672 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 51:
#line 457 "./config/rx-parse.y" /* yacc.c:1648  */
    { B2 (0x78, 0); FE ((yyvsp[-2].exp), 7, 5); F ((yyvsp[0].regno), 12, 4); }
#line 2678 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 52:
#line 459 "./config/rx-parse.y" /* yacc.c:1648  */
    { B2 (0x7a, 0); FE ((yyvsp[-2].exp), 7, 5); F ((yyvsp[0].regno), 12, 4); }
#line 2684 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 53:
#line 464 "./config/rx-parse.y" /* yacc.c:1648  */
    { B2 (0x7c, 0x00); FE ((yyvsp[-2].exp), 7, 5); F ((yyvsp[0].regno), 12, 4); }
#line 2690 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 54:
#line 469 "./config/rx-parse.y" /* yacc.c:1648  */
    { B2 (0x7e, 0x30); F ((yyvsp[0].regno), 12, 4); }
#line 2696 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 55:
#line 471 "./config/rx-parse.y" /* yacc.c:1648  */
    { B2 (0x7e, 0x40); F ((yyvsp[0].regno), 12, 4); }
#line 2702 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 56:
#line 473 "./config/rx-parse.y" /* yacc.c:1648  */
    { B2 (0x7e, 0x50); F ((yyvsp[0].regno), 12, 4); }
#line 2708 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 57:
#line 478 "./config/rx-parse.y" /* yacc.c:1648  */
    { B2 (0x7e, 0x80); F ((yyvsp[-1].regno), 10, 2); F ((yyvsp[0].regno), 12, 4); }
#line 2714 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 58:
#line 483 "./config/rx-parse.y" /* yacc.c:1648  */
    { B2 (0x7e, 0xb0); F ((yyvsp[0].regno), 12, 4); }
#line 2720 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 59:
#line 488 "./config/rx-parse.y" /* yacc.c:1648  */
    { if ((yyvsp[0].regno) == 13)
	      { rx_check_v2 (); }
	    if ((yyvsp[0].regno) < 16)
	      { B2 (0x7e, 0xc0); F ((yyvsp[0].regno), 12, 4); }
	    else
	      as_bad (_("PUSHC can only push the first 16 control registers")); }
#line 2731 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 60:
#line 498 "./config/rx-parse.y" /* yacc.c:1648  */
    { if ((yyvsp[0].regno) == 13)
	    { rx_check_v2 (); }
	    if ((yyvsp[0].regno) < 16)
	      { B2 (0x7e, 0xe0); F ((yyvsp[0].regno), 12, 4); }
	    else
	      as_bad (_("POPC can only pop the first 16 control registers")); }
#line 2742 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 61:
#line 508 "./config/rx-parse.y" /* yacc.c:1648  */
    { B2 (0x7f, 0xa0); F ((yyvsp[0].regno), 12, 4); }
#line 2748 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 62:
#line 510 "./config/rx-parse.y" /* yacc.c:1648  */
    { B2 (0x7f, 0xb0); F ((yyvsp[0].regno), 12, 4); }
#line 2754 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 63:
#line 515 "./config/rx-parse.y" /* yacc.c:1648  */
    { B2 (0x7f, 0x00); F ((yyvsp[0].regno), 12, 4); }
#line 2760 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 64:
#line 517 "./config/rx-parse.y" /* yacc.c:1648  */
    { B2 (0x7f, 0x10); F ((yyvsp[0].regno), 12, 4); }
#line 2766 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 65:
#line 519 "./config/rx-parse.y" /* yacc.c:1648  */
    { B2 (0x7f, 0x40); F ((yyvsp[0].regno), 12, 4); }
#line 2772 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 66:
#line 521 "./config/rx-parse.y" /* yacc.c:1648  */
    { B2 (0x7f, 0x50); F ((yyvsp[0].regno), 12, 4); }
#line 2778 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 67:
#line 526 "./config/rx-parse.y" /* yacc.c:1648  */
    { B2 (0x7f, 0x83); rx_note_string_insn_use (); }
#line 2784 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 68:
#line 528 "./config/rx-parse.y" /* yacc.c:1648  */
    { B2 (0x7f, 0x87); rx_note_string_insn_use (); }
#line 2790 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 69:
#line 530 "./config/rx-parse.y" /* yacc.c:1648  */
    { B2 (0x7f, 0x8b); rx_note_string_insn_use (); }
#line 2796 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 70:
#line 532 "./config/rx-parse.y" /* yacc.c:1648  */
    { B2 (0x7f, 0x8f); rx_note_string_insn_use (); }
#line 2802 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 71:
#line 537 "./config/rx-parse.y" /* yacc.c:1648  */
    { B2 (0x7f, 0x80); F ((yyvsp[0].regno), 14, 2); rx_note_string_insn_use (); }
#line 2808 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 72:
#line 539 "./config/rx-parse.y" /* yacc.c:1648  */
    { B2 (0x7f, 0x84); F ((yyvsp[0].regno), 14, 2); rx_note_string_insn_use (); }
#line 2814 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 73:
#line 541 "./config/rx-parse.y" /* yacc.c:1648  */
    { B2 (0x7f, 0x88); F ((yyvsp[0].regno), 14, 2); }
#line 2820 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 74:
#line 546 "./config/rx-parse.y" /* yacc.c:1648  */
    { B2 (0x7f, 0x8c); F ((yyvsp[0].regno), 14, 2); rx_note_string_insn_use (); }
#line 2826 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 75:
#line 551 "./config/rx-parse.y" /* yacc.c:1648  */
    { B2 (0x7f, 0x94); }
#line 2832 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 76:
#line 553 "./config/rx-parse.y" /* yacc.c:1648  */
    { B2 (0x7f, 0x95); }
#line 2838 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 77:
#line 555 "./config/rx-parse.y" /* yacc.c:1648  */
    { B2 (0x7f, 0x96); }
#line 2844 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 78:
#line 557 "./config/rx-parse.y" /* yacc.c:1648  */
    { B2 (0x7f, 0x93); }
#line 2850 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 79:
#line 562 "./config/rx-parse.y" /* yacc.c:1648  */
    { B3 (0x75, 0x70, 0x00); FE ((yyvsp[0].exp), 20, 4); }
#line 2856 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 80:
#line 568 "./config/rx-parse.y" /* yacc.c:1648  */
    { if ((yyvsp[-5].regno) <= 7 && (yyvsp[-1].regno) <= 7 && rx_disp5op (&(yyvsp[-3].exp), (yyvsp[-6].regno)))
	      { B2 (0x80, 0); F ((yyvsp[-6].regno), 2, 2); F ((yyvsp[-1].regno), 9, 3); F ((yyvsp[-5].regno), 13, 3); rx_field5s ((yyvsp[-3].exp)); }
	    else
	      { B2 (0xc3, 0x00); F ((yyvsp[-6].regno), 2, 2); F ((yyvsp[-1].regno), 8, 4); F ((yyvsp[-5].regno), 12, 4); DSP ((yyvsp[-3].exp), 4, (yyvsp[-6].regno)); }}
#line 2865 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 81:
#line 576 "./config/rx-parse.y" /* yacc.c:1648  */
    { if ((yyvsp[-3].regno) <= 7 && (yyvsp[0].regno) <= 7 && rx_disp5op (&(yyvsp[-5].exp), (yyvsp[-6].regno)))
	      { B2 (0x88, 0); F ((yyvsp[-6].regno), 2, 2); F ((yyvsp[-3].regno), 9, 3); F ((yyvsp[0].regno), 13, 3); rx_field5s ((yyvsp[-5].exp)); }
	    else
	      { B2 (0xcc, 0x00); F ((yyvsp[-6].regno), 2, 2); F ((yyvsp[-3].regno), 8, 4); F ((yyvsp[0].regno), 12, 4); DSP ((yyvsp[-5].exp), 6, (yyvsp[-6].regno)); } }
#line 2874 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 82:
#line 590 "./config/rx-parse.y" /* yacc.c:1648  */
    { B2 (0xc3, 0x00); F ((yyvsp[-5].regno), 2, 2); F ((yyvsp[-1].regno), 8, 4); F ((yyvsp[-4].regno), 12, 4); }
#line 2880 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 83:
#line 595 "./config/rx-parse.y" /* yacc.c:1648  */
    { B2 (0xc0, 0); F ((yyvsp[-8].regno), 2, 2); F ((yyvsp[-6].regno), 8, 4); F ((yyvsp[-1].regno), 12, 4); DSP ((yyvsp[-3].exp), 4, (yyvsp[-8].regno)); }
#line 2886 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 84:
#line 600 "./config/rx-parse.y" /* yacc.c:1648  */
    { B2 (0xc0, 0x00); F ((yyvsp[-9].regno), 2, 2); F ((yyvsp[-6].regno), 8, 4); F ((yyvsp[-1].regno), 12, 4); DSP ((yyvsp[-8].exp), 6, (yyvsp[-9].regno)); DSP ((yyvsp[-3].exp), 4, (yyvsp[-9].regno)); }
#line 2892 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 85:
#line 605 "./config/rx-parse.y" /* yacc.c:1648  */
    { B2 (0xcf, 0x00); F ((yyvsp[-3].regno), 2, 2); F ((yyvsp[-2].regno), 8, 4); F ((yyvsp[0].regno), 12, 4); }
#line 2898 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 86:
#line 610 "./config/rx-parse.y" /* yacc.c:1648  */
    { B2 (0xcc, 0x00); F ((yyvsp[-5].regno), 2, 2); F ((yyvsp[-3].regno), 8, 4); F ((yyvsp[0].regno), 12, 4); }
#line 2904 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 87:
#line 615 "./config/rx-parse.y" /* yacc.c:1648  */
    { B2 (0xf0, 0x00); F ((yyvsp[-2].regno), 8, 4); FE ((yyvsp[-6].exp), 13, 3); DSP ((yyvsp[-4].exp), 6, BSIZE); }
#line 2910 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 88:
#line 617 "./config/rx-parse.y" /* yacc.c:1648  */
    { B2 (0xf0, 0x08); F ((yyvsp[-2].regno), 8, 4); FE ((yyvsp[-6].exp), 13, 3); DSP ((yyvsp[-4].exp), 6, BSIZE); }
#line 2916 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 89:
#line 619 "./config/rx-parse.y" /* yacc.c:1648  */
    { B2 (0xf4, 0x00); F ((yyvsp[-2].regno), 8, 4); FE ((yyvsp[-6].exp), 13, 3); DSP ((yyvsp[-4].exp), 6, BSIZE); }
#line 2922 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 90:
#line 624 "./config/rx-parse.y" /* yacc.c:1648  */
    { B2 (0xf4, 0x08); F ((yyvsp[-4].regno), 14, 2); F ((yyvsp[-1].regno), 8, 4); DSP ((yyvsp[-3].exp), 6, (yyvsp[-4].regno)); }
#line 2928 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 91:
#line 628 "./config/rx-parse.y" /* yacc.c:1648  */
    { sub_op = 0; }
#line 2934 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 93:
#line 629 "./config/rx-parse.y" /* yacc.c:1648  */
    { sub_op = 1; sub_op2 = 1; }
#line 2940 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 95:
#line 630 "./config/rx-parse.y" /* yacc.c:1648  */
    { sub_op = 2; }
#line 2946 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 97:
#line 631 "./config/rx-parse.y" /* yacc.c:1648  */
    { sub_op = 3; sub_op2 = 2; }
#line 2952 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 99:
#line 632 "./config/rx-parse.y" /* yacc.c:1648  */
    { sub_op = 4; }
#line 2958 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 101:
#line 633 "./config/rx-parse.y" /* yacc.c:1648  */
    { sub_op = 5; }
#line 2964 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 103:
#line 634 "./config/rx-parse.y" /* yacc.c:1648  */
    { sub_op = 6; }
#line 2970 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 105:
#line 635 "./config/rx-parse.y" /* yacc.c:1648  */
    { sub_op = 7; }
#line 2976 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 107:
#line 636 "./config/rx-parse.y" /* yacc.c:1648  */
    { sub_op = 8; }
#line 2982 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 109:
#line 637 "./config/rx-parse.y" /* yacc.c:1648  */
    { sub_op = 9; }
#line 2988 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 111:
#line 638 "./config/rx-parse.y" /* yacc.c:1648  */
    { sub_op = 12; }
#line 2994 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 113:
#line 639 "./config/rx-parse.y" /* yacc.c:1648  */
    { sub_op = 13; }
#line 3000 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 115:
#line 640 "./config/rx-parse.y" /* yacc.c:1648  */
    { sub_op = 14; sub_op2 = 0; }
#line 3006 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 117:
#line 641 "./config/rx-parse.y" /* yacc.c:1648  */
    { sub_op = 14; sub_op2 = 0; }
#line 3012 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 119:
#line 642 "./config/rx-parse.y" /* yacc.c:1648  */
    { sub_op = 15; sub_op2 = 1; }
#line 3018 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 121:
#line 646 "./config/rx-parse.y" /* yacc.c:1648  */
    { sub_op = 6; }
#line 3024 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 123:
#line 647 "./config/rx-parse.y" /* yacc.c:1648  */
    { sub_op = 7; }
#line 3030 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 125:
#line 648 "./config/rx-parse.y" /* yacc.c:1648  */
    { sub_op = 16; }
#line 3036 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 127:
#line 649 "./config/rx-parse.y" /* yacc.c:1648  */
    { sub_op = 17; }
#line 3042 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 129:
#line 650 "./config/rx-parse.y" /* yacc.c:1648  */
    { sub_op = 21; }
#line 3048 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 131:
#line 655 "./config/rx-parse.y" /* yacc.c:1648  */
    { id24 (1, 0x63, 0x00); F ((yyvsp[0].regno), 16, 4); F ((yyvsp[-2].regno), 20, 4); }
#line 3054 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 132:
#line 657 "./config/rx-parse.y" /* yacc.c:1648  */
    { id24 (1, 0x67, 0x00); F ((yyvsp[0].regno), 16, 4); F ((yyvsp[-2].regno), 20, 4); }
#line 3060 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 133:
#line 659 "./config/rx-parse.y" /* yacc.c:1648  */
    { id24 (1, 0x6b, 0x00); F ((yyvsp[0].regno), 16, 4); F ((yyvsp[-2].regno), 20, 4); }
#line 3066 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 134:
#line 661 "./config/rx-parse.y" /* yacc.c:1648  */
    { id24 (1, 0x6f, 0x00); F ((yyvsp[0].regno), 16, 4); F ((yyvsp[-2].regno), 20, 4); }
#line 3072 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 135:
#line 664 "./config/rx-parse.y" /* yacc.c:1648  */
    { id24 (1, 0x60, 0x00); F ((yyvsp[-2].regno), 16, 4); F ((yyvsp[-6].regno), 20, 4); DSP ((yyvsp[-4].exp), 14, BSIZE); }
#line 3078 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 136:
#line 666 "./config/rx-parse.y" /* yacc.c:1648  */
    { id24 (1, 0x64, 0x00); F ((yyvsp[-2].regno), 16, 4); F ((yyvsp[-6].regno), 20, 4); DSP ((yyvsp[-4].exp), 14, BSIZE); }
#line 3084 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 137:
#line 668 "./config/rx-parse.y" /* yacc.c:1648  */
    { id24 (1, 0x68, 0x00); F ((yyvsp[-2].regno), 16, 4); F ((yyvsp[-6].regno), 20, 4); DSP ((yyvsp[-4].exp), 14, BSIZE); }
#line 3090 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 138:
#line 670 "./config/rx-parse.y" /* yacc.c:1648  */
    { id24 (1, 0x6c, 0x00); F ((yyvsp[-2].regno), 16, 4); F ((yyvsp[-6].regno), 20, 4); DSP ((yyvsp[-4].exp), 14, BSIZE); }
#line 3096 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 139:
#line 674 "./config/rx-parse.y" /* yacc.c:1648  */
    { sub_op = 0; }
#line 3102 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 141:
#line 675 "./config/rx-parse.y" /* yacc.c:1648  */
    { sub_op = 1; }
#line 3108 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 143:
#line 676 "./config/rx-parse.y" /* yacc.c:1648  */
    { sub_op = 2; }
#line 3114 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 145:
#line 677 "./config/rx-parse.y" /* yacc.c:1648  */
    { sub_op = 3; }
#line 3120 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 147:
#line 678 "./config/rx-parse.y" /* yacc.c:1648  */
    { sub_op = 4; }
#line 3126 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 149:
#line 679 "./config/rx-parse.y" /* yacc.c:1648  */
    { sub_op = 8; }
#line 3132 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 151:
#line 680 "./config/rx-parse.y" /* yacc.c:1648  */
    { sub_op = 5; }
#line 3138 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 153:
#line 681 "./config/rx-parse.y" /* yacc.c:1648  */
    { sub_op = 9; }
#line 3144 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 155:
#line 682 "./config/rx-parse.y" /* yacc.c:1648  */
    { sub_op = 6; }
#line 3150 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 157:
#line 690 "./config/rx-parse.y" /* yacc.c:1648  */
    { id24 (1, 0xdb, 0x00); F ((yyvsp[-2].regno), 20, 4); F ((yyvsp[0].regno), 16, 4); }
#line 3156 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 158:
#line 692 "./config/rx-parse.y" /* yacc.c:1648  */
    { id24 (1, 0xd0, 0x00); F ((yyvsp[-5].regno), 20, 4); F ((yyvsp[-4].regno), 12, 2); F ((yyvsp[-1].regno), 16, 4); DSP ((yyvsp[-3].exp), 14, (yyvsp[-4].regno)); }
#line 3162 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 159:
#line 697 "./config/rx-parse.y" /* yacc.c:1648  */
    { id24 (1, 0xe0, 0x00); F ((yyvsp[-8].regno), 20, 4); FE ((yyvsp[-6].exp), 11, 3);
	      F ((yyvsp[-2].regno), 16, 4); DSP ((yyvsp[-4].exp), 14, BSIZE); }
#line 3169 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 160:
#line 703 "./config/rx-parse.y" /* yacc.c:1648  */
    { id24 (1, 0xe0, 0x0f); FE ((yyvsp[-6].exp), 11, 3); F ((yyvsp[-2].regno), 16, 4);
	      DSP ((yyvsp[-4].exp), 14, BSIZE); }
#line 3176 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 161:
#line 709 "./config/rx-parse.y" /* yacc.c:1648  */
    { id24 (2, 0x00, 0x00); F ((yyvsp[-2].regno), 16, 4); F ((yyvsp[0].regno), 20, 4); }
#line 3182 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 162:
#line 711 "./config/rx-parse.y" /* yacc.c:1648  */
    { rx_check_v2 (); id24 (2, 0x00, 0x00); F ((yyvsp[-4].regno), 16, 4); F ((yyvsp[-2].regno), 20, 4); F ((yyvsp[0].regno), 12, 1); }
#line 3188 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 163:
#line 713 "./config/rx-parse.y" /* yacc.c:1648  */
    { id24 (2, 0x01, 0x00); F ((yyvsp[-2].regno), 16, 4); F ((yyvsp[0].regno), 20, 4); }
#line 3194 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 164:
#line 715 "./config/rx-parse.y" /* yacc.c:1648  */
    { rx_check_v2 (); id24 (2, 0x01, 0x00); F ((yyvsp[-4].regno), 16, 4); F ((yyvsp[-2].regno), 20, 4); F ((yyvsp[0].regno), 12, 1); }
#line 3200 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 165:
#line 717 "./config/rx-parse.y" /* yacc.c:1648  */
    { id24 (2, 0x04, 0x00); F ((yyvsp[-2].regno), 16, 4); F ((yyvsp[0].regno), 20, 4); }
#line 3206 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 166:
#line 719 "./config/rx-parse.y" /* yacc.c:1648  */
    { rx_check_v2 (); id24 (2, 0x04, 0x00); F ((yyvsp[-4].regno), 16, 4); F ((yyvsp[-2].regno), 20, 4); F ((yyvsp[0].regno), 12, 1); }
#line 3212 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 167:
#line 721 "./config/rx-parse.y" /* yacc.c:1648  */
    { id24 (2, 0x05, 0x00); F ((yyvsp[-2].regno), 16, 4); F ((yyvsp[0].regno), 20, 4); }
#line 3218 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 168:
#line 723 "./config/rx-parse.y" /* yacc.c:1648  */
    { rx_check_v2 (); id24 (2, 0x05, 0x00); F ((yyvsp[-4].regno), 16, 4); F ((yyvsp[-2].regno), 20, 4); F ((yyvsp[0].regno), 12, 1); }
#line 3224 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 169:
#line 729 "./config/rx-parse.y" /* yacc.c:1648  */
    { id24 (2, 0x17, 0x00); F ((yyvsp[0].regno), 20, 4); }
#line 3230 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 170:
#line 731 "./config/rx-parse.y" /* yacc.c:1648  */
    { rx_check_v2 (); id24 (2, 0x17, 0x00); F ((yyvsp[-2].regno), 20, 4); F ((yyvsp[0].regno), 16, 1); }
#line 3236 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 171:
#line 733 "./config/rx-parse.y" /* yacc.c:1648  */
    { id24 (2, 0x17, 0x10); F ((yyvsp[0].regno), 20, 4); }
#line 3242 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 172:
#line 735 "./config/rx-parse.y" /* yacc.c:1648  */
    { rx_check_v2 (); id24 (2, 0x17, 0x10); F ((yyvsp[-2].regno), 20, 4); F ((yyvsp[0].regno), 16, 1); }
#line 3248 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 173:
#line 737 "./config/rx-parse.y" /* yacc.c:1648  */
    { id24 (2, 0x1f, 0x00); F ((yyvsp[0].regno), 20, 4); }
#line 3254 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 174:
#line 738 "./config/rx-parse.y" /* yacc.c:1648  */
    { sub_op = 0; }
#line 3260 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 176:
#line 740 "./config/rx-parse.y" /* yacc.c:1648  */
    { id24 (2, 0x1f, 0x20); F ((yyvsp[0].regno), 20, 4); }
#line 3266 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 177:
#line 741 "./config/rx-parse.y" /* yacc.c:1648  */
    { sub_op = 2; }
#line 3272 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 179:
#line 743 "./config/rx-parse.y" /* yacc.c:1648  */
    { id24 (2, 0x1f, 0x10); F ((yyvsp[0].regno), 20, 4); }
#line 3278 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 180:
#line 744 "./config/rx-parse.y" /* yacc.c:1648  */
    { sub_op = 1; }
#line 3284 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 182:
#line 746 "./config/rx-parse.y" /* yacc.c:1648  */
    { id24 (2, 0x18, 0x00);
	    if (rx_uintop ((yyvsp[0].exp), 4) && exp_val((yyvsp[0].exp)) == 1)
	      ;
	    else if (rx_uintop ((yyvsp[0].exp), 4) && exp_val((yyvsp[0].exp)) == 2)
	      F (1, 19, 1);
	    else
	      as_bad (_("RACW expects #1 or #2"));}
#line 3296 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 183:
#line 754 "./config/rx-parse.y" /* yacc.c:1648  */
    { rx_check_v2 (); id24 (2, 0x18, 0x00); F ((yyvsp[0].regno), 16, 1);
	    if (rx_uintop ((yyvsp[-2].exp), 4) && exp_val((yyvsp[-2].exp)) == 1)
	      ;
	    else if (rx_uintop ((yyvsp[-2].exp), 4) && exp_val((yyvsp[-2].exp)) == 2)
	      F (1, 19, 1);
	    else
	      as_bad (_("RACW expects #1 or #2"));}
#line 3308 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 184:
#line 765 "./config/rx-parse.y" /* yacc.c:1648  */
    { id24 (2, 0x20, 0); F ((yyvsp[-6].regno), 14, 2); F ((yyvsp[-2].regno), 16, 4); F ((yyvsp[-5].regno), 20, 4); }
#line 3314 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 185:
#line 767 "./config/rx-parse.y" /* yacc.c:1648  */
    { id24 (2, 0x24, 0); F ((yyvsp[-6].regno), 14, 2); F ((yyvsp[-1].regno), 16, 4); F ((yyvsp[-5].regno), 20, 4); }
#line 3320 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 186:
#line 772 "./config/rx-parse.y" /* yacc.c:1648  */
    { id24 (2, 0x28, 0); F ((yyvsp[-6].regno), 14, 2); F ((yyvsp[-4].regno), 16, 4); F ((yyvsp[0].regno), 20, 4); }
#line 3326 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 187:
#line 774 "./config/rx-parse.y" /* yacc.c:1648  */
    { id24 (2, 0x2c, 0); F ((yyvsp[-6].regno), 14, 2); F ((yyvsp[-3].regno), 16, 4); F ((yyvsp[0].regno), 20, 4); }
#line 3332 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 188:
#line 779 "./config/rx-parse.y" /* yacc.c:1648  */
    { id24 (2, 0x38, 0); F ((yyvsp[-6].regno), 15, 1); F ((yyvsp[-4].regno), 16, 4); F ((yyvsp[0].regno), 20, 4); }
#line 3338 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 189:
#line 781 "./config/rx-parse.y" /* yacc.c:1648  */
    { id24 (2, 0x3c, 0); F ((yyvsp[-6].regno), 15, 1); F ((yyvsp[-3].regno), 16, 4); F ((yyvsp[0].regno), 20, 4); }
#line 3344 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 190:
#line 785 "./config/rx-parse.y" /* yacc.c:1648  */
    { sub_op = 6; }
#line 3350 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 192:
#line 786 "./config/rx-parse.y" /* yacc.c:1648  */
    { sub_op = 4; }
#line 3356 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 194:
#line 787 "./config/rx-parse.y" /* yacc.c:1648  */
    { sub_op = 5; }
#line 3362 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 196:
#line 788 "./config/rx-parse.y" /* yacc.c:1648  */
    { sub_op = 7; }
#line 3368 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 198:
#line 793 "./config/rx-parse.y" /* yacc.c:1648  */
    { if ((yyvsp[0].regno) == 13)
	      rx_check_v2 ();
	  id24 (2, 0x68, 0x00); F ((yyvsp[0].regno) % 16, 20, 4); F ((yyvsp[0].regno) / 16, 15, 1);
	    F ((yyvsp[-2].regno), 16, 4); }
#line 3377 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 199:
#line 801 "./config/rx-parse.y" /* yacc.c:1648  */
    { if ((yyvsp[-2].regno) == 13)
	    rx_check_v2 ();
	  id24 (2, 0x6a, 0); F ((yyvsp[-2].regno), 15, 5); F ((yyvsp[0].regno), 20, 4); }
#line 3385 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 200:
#line 808 "./config/rx-parse.y" /* yacc.c:1648  */
    { id24 (2, 0x6e, 0); FE ((yyvsp[-2].exp), 15, 5); F ((yyvsp[0].regno), 20, 4); }
#line 3391 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 201:
#line 810 "./config/rx-parse.y" /* yacc.c:1648  */
    { id24 (2, 0x6c, 0); FE ((yyvsp[-2].exp), 15, 5); F ((yyvsp[0].regno), 20, 4); }
#line 3397 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 202:
#line 815 "./config/rx-parse.y" /* yacc.c:1648  */
    { if ((yyvsp[0].regno) == 13)
	      rx_check_v2 ();
	    id24 (2, 0x73, 0x00); F ((yyvsp[0].regno), 19, 5); IMM ((yyvsp[-2].exp), 12); }
#line 3405 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 203:
#line 822 "./config/rx-parse.y" /* yacc.c:1648  */
    { id24 (2, 0xe0, 0x00); F ((yyvsp[-4].regno), 16, 4); FE ((yyvsp[-2].exp), 11, 5);
	      F ((yyvsp[0].regno), 20, 4); }
#line 3412 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 204:
#line 828 "./config/rx-parse.y" /* yacc.c:1648  */
    { id24 (2, 0xe0, 0xf0); FE ((yyvsp[-2].exp), 11, 5); F ((yyvsp[0].regno), 20, 4); }
#line 3418 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 205:
#line 833 "./config/rx-parse.y" /* yacc.c:1648  */
    { id24 (3, 0x00, 0); F ((yyvsp[-7].regno), 10, 2); F ((yyvsp[-3].regno), 12, 4); F ((yyvsp[-1].regno), 16, 4); F ((yyvsp[-6].regno), 20, 4); }
#line 3424 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 206:
#line 836 "./config/rx-parse.y" /* yacc.c:1648  */
    { id24 (3, 0x40, 0); F ((yyvsp[-7].regno), 10, 2); F ((yyvsp[-5].regno), 12, 4); F ((yyvsp[-3].regno), 16, 4); F ((yyvsp[0].regno), 20, 4); }
#line 3430 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 207:
#line 839 "./config/rx-parse.y" /* yacc.c:1648  */
    { id24 (3, 0xc0, 0); F ((yyvsp[-7].regno), 10, 2); F ((yyvsp[-5].regno), 12, 4); F ((yyvsp[-3].regno), 16, 4); F ((yyvsp[0].regno), 20, 4); }
#line 3436 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 208:
#line 843 "./config/rx-parse.y" /* yacc.c:1648  */
    { sub_op = 0; }
#line 3442 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 210:
#line 844 "./config/rx-parse.y" /* yacc.c:1648  */
    { sub_op = 2; }
#line 3448 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 212:
#line 845 "./config/rx-parse.y" /* yacc.c:1648  */
    { sub_op = 3; }
#line 3454 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 214:
#line 846 "./config/rx-parse.y" /* yacc.c:1648  */
    { sub_op = 4; }
#line 3460 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 216:
#line 847 "./config/rx-parse.y" /* yacc.c:1648  */
    { sub_op = 5; }
#line 3466 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 218:
#line 853 "./config/rx-parse.y" /* yacc.c:1648  */
    { id24 (2, 0x70, 0x20); F ((yyvsp[0].regno), 20, 4); NBIMM ((yyvsp[-2].exp), 12); }
#line 3472 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 219:
#line 858 "./config/rx-parse.y" /* yacc.c:1648  */
    { rx_check_v2 (); B3 (0xfd, 0x27, 0x00); F ((yyvsp[-1].regno), 16, 4); F ((yyvsp[-4].regno), 20, 4); }
#line 3478 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 220:
#line 863 "./config/rx-parse.y" /* yacc.c:1648  */
    { rx_check_v2 (); B3 (0xfd, 0x2f, 0x00); F ((yyvsp[-3].regno), 16, 4); F ((yyvsp[0].regno), 20, 4); }
#line 3484 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 221:
#line 868 "./config/rx-parse.y" /* yacc.c:1648  */
    { rx_check_v2 (); id24 (2, 0x07, 0x00); F ((yyvsp[-4].regno), 16, 4); F ((yyvsp[-2].regno), 20, 4); F ((yyvsp[0].regno), 12, 1); }
#line 3490 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 222:
#line 870 "./config/rx-parse.y" /* yacc.c:1648  */
    { rx_check_v2 (); id24 (2, 0x47, 0x00); F ((yyvsp[-4].regno), 16, 4); F ((yyvsp[-2].regno), 20, 4); F ((yyvsp[0].regno), 12, 1); }
#line 3496 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 223:
#line 872 "./config/rx-parse.y" /* yacc.c:1648  */
    { rx_check_v2 (); id24 (2, 0x03, 0x00); F ((yyvsp[-4].regno), 16, 4); F ((yyvsp[-2].regno), 20, 4); F ((yyvsp[0].regno), 12, 1); }
#line 3502 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 224:
#line 874 "./config/rx-parse.y" /* yacc.c:1648  */
    { rx_check_v2 (); id24 (2, 0x06, 0x00); F ((yyvsp[-4].regno), 16, 4); F ((yyvsp[-2].regno), 20, 4); F ((yyvsp[0].regno), 12, 1); }
#line 3508 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 225:
#line 876 "./config/rx-parse.y" /* yacc.c:1648  */
    { rx_check_v2 (); id24 (2, 0x44, 0x00); F ((yyvsp[-4].regno), 16, 4); F ((yyvsp[-2].regno), 20, 4); F ((yyvsp[0].regno), 12, 1); }
#line 3514 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 226:
#line 878 "./config/rx-parse.y" /* yacc.c:1648  */
    { rx_check_v2 (); id24 (2, 0x46, 0x00); F ((yyvsp[-4].regno), 16, 4); F ((yyvsp[-2].regno), 20, 4); F ((yyvsp[0].regno), 12, 1); }
#line 3520 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 227:
#line 880 "./config/rx-parse.y" /* yacc.c:1648  */
    { rx_check_v2 (); id24 (2, 0x45, 0x00); F ((yyvsp[-4].regno), 16, 4); F ((yyvsp[-2].regno), 20, 4); F ((yyvsp[0].regno), 12, 1); }
#line 3526 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 228:
#line 882 "./config/rx-parse.y" /* yacc.c:1648  */
    { rx_check_v2 (); id24 (2, 0x02, 0x00); F ((yyvsp[-4].regno), 16, 4); F ((yyvsp[-2].regno), 20, 4); F ((yyvsp[0].regno), 12, 1); }
#line 3532 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 229:
#line 883 "./config/rx-parse.y" /* yacc.c:1648  */
    { sub_op = 3; }
#line 3538 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 231:
#line 885 "./config/rx-parse.y" /* yacc.c:1648  */
    { rx_check_v2 (); id24 (2, 0x17, 0x30); F ((yyvsp[0].regno), 16, 1); F ((yyvsp[-2].regno), 20, 4); }
#line 3544 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 232:
#line 887 "./config/rx-parse.y" /* yacc.c:1648  */
    { rx_check_v2 (); id24 (2, 0x19, 0x00); F ((yyvsp[0].regno), 16, 1);
	    if (rx_uintop ((yyvsp[-2].exp), 4) && (yyvsp[-2].exp).X_add_number == 1)
	      ;
	    else if (rx_uintop ((yyvsp[-2].exp), 4) && (yyvsp[-2].exp).X_add_number == 2)
	      F (1, 19, 1);
	    else
	      as_bad (_("RACL expects #1 or #2"));}
#line 3556 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 233:
#line 895 "./config/rx-parse.y" /* yacc.c:1648  */
    { rx_check_v2 (); id24 (2, 0x19, 0x40); F ((yyvsp[0].regno), 16, 1);
	    if (rx_uintop ((yyvsp[-2].exp), 4) && (yyvsp[-2].exp).X_add_number == 1)
	      ;
	    else if (rx_uintop ((yyvsp[-2].exp), 4) && (yyvsp[-2].exp).X_add_number == 2)
	      F (1, 19, 1);
	    else
	      as_bad (_("RDACL expects #1 or #2"));}
#line 3568 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 234:
#line 903 "./config/rx-parse.y" /* yacc.c:1648  */
    { rx_check_v2 (); id24 (2, 0x18, 0x40); F ((yyvsp[0].regno), 16, 1);
	    if (rx_uintop ((yyvsp[-2].exp), 4) && (yyvsp[-2].exp).X_add_number == 1)
	      ;
	    else if (rx_uintop ((yyvsp[-2].exp), 4) && (yyvsp[-2].exp).X_add_number == 2)
	      F (1, 19, 1);
	    else
	      as_bad (_("RDACW expects #1 or #2"));}
#line 3580 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 235:
#line 912 "./config/rx-parse.y" /* yacc.c:1648  */
    { rx_check_v3(); sub_op = 1; }
#line 3586 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 237:
#line 913 "./config/rx-parse.y" /* yacc.c:1648  */
    { rx_check_v3(); sub_op = 0; }
#line 3592 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 239:
#line 916 "./config/rx-parse.y" /* yacc.c:1648  */
    { rx_check_v3(); sub_op = 1; }
#line 3598 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 241:
#line 917 "./config/rx-parse.y" /* yacc.c:1648  */
    { rx_check_v3(); sub_op = 0; }
#line 3604 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 243:
#line 920 "./config/rx-parse.y" /* yacc.c:1648  */
    { rx_check_dfpu(); sub_op = 0x0c; sub_op2 = 0x01; }
#line 3610 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 245:
#line 921 "./config/rx-parse.y" /* yacc.c:1648  */
    { rx_check_dfpu(); sub_op = 0x0c; sub_op2 = 0x02; }
#line 3616 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 247:
#line 922 "./config/rx-parse.y" /* yacc.c:1648  */
    { rx_check_dfpu(); sub_op = 0x0d; sub_op2 = 0x0d; }
#line 3622 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 249:
#line 923 "./config/rx-parse.y" /* yacc.c:1648  */
    { rx_check_dfpu(); sub_op = 0x0d; sub_op2 = 0x00; }
#line 3628 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 251:
#line 924 "./config/rx-parse.y" /* yacc.c:1648  */
    { rx_check_dfpu(); sub_op = 0x0d; sub_op2 = 0x0c; }
#line 3634 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 253:
#line 925 "./config/rx-parse.y" /* yacc.c:1648  */
    { rx_check_dfpu(); sub_op = 0x0d; sub_op2 = 0x08;}
#line 3640 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 255:
#line 926 "./config/rx-parse.y" /* yacc.c:1648  */
    { rx_check_dfpu(); sub_op = 0x0d; sub_op2 = 0x09; }
#line 3646 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 257:
#line 927 "./config/rx-parse.y" /* yacc.c:1648  */
    { rx_check_dfpu(); sub_op = 0x00; }
#line 3652 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 259:
#line 928 "./config/rx-parse.y" /* yacc.c:1648  */
    { rx_check_dfpu(); sub_op = 0x05; }
#line 3658 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 261:
#line 929 "./config/rx-parse.y" /* yacc.c:1648  */
    { rx_check_dfpu(); sub_op = 0x02; }
#line 3664 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 263:
#line 930 "./config/rx-parse.y" /* yacc.c:1648  */
    { rx_check_dfpu(); sub_op = 0x01; }
#line 3670 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 265:
#line 931 "./config/rx-parse.y" /* yacc.c:1648  */
    { rx_check_dfpu();
	    B4(0x76, 0x90, 0x08, 0x00); F((yyvsp[-3].regno), 24, 4); F((yyvsp[-2].regno), 28, 4); F((yyvsp[0].regno), 16, 4); }
#line 3677 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 266:
#line 934 "./config/rx-parse.y" /* yacc.c:1648  */
    { rx_check_dfpu();
	  B4(0xfd, 0x77, 0x80, 0x03); F((yyvsp[-2].regno), 20, 4); F((yyvsp[0].regno), 24, 4); }
#line 3684 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 267:
#line 937 "./config/rx-parse.y" /* yacc.c:1648  */
    { rx_check_dfpu();
	  B4(0xfd, 0x77, 0x80, 0x02); F((yyvsp[-2].regno), 20, 4); F((yyvsp[0].regno), 24, 4); }
#line 3691 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 268:
#line 940 "./config/rx-parse.y" /* yacc.c:1648  */
    { rx_check_dfpu();
	  B4(0xfd, 0x77, 0x80, 0x00); F((yyvsp[-2].regno), 20, 4); F((yyvsp[0].regno), 24, 4); }
#line 3698 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 269:
#line 943 "./config/rx-parse.y" /* yacc.c:1648  */
    { rx_check_dfpu();
	  B4(0xfd, 0x75, 0x80, 0x02); F((yyvsp[-2].regno), 24, 4); F((yyvsp[0].regno), 20, 4); }
#line 3705 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 270:
#line 946 "./config/rx-parse.y" /* yacc.c:1648  */
    { rx_check_dfpu();
	  B4(0xfd, 0x75, 0x80, 0x00); F((yyvsp[-2].regno), 24, 4); F((yyvsp[0].regno), 20, 4); }
#line 3712 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 271:
#line 949 "./config/rx-parse.y" /* yacc.c:1648  */
    { rx_check_dfpu();
	  B4(0x76, 0x90, 0x0c, 0x00); F((yyvsp[-2].regno), 16, 4); F((yyvsp[0].regno), 24, 4); }
#line 3719 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 272:
#line 952 "./config/rx-parse.y" /* yacc.c:1648  */
    { rx_check_dfpu();
	  B4(0xfc, 0x78, 0x08, 0x00); F((yyvsp[-1].regno), 16, 4); F((yyvsp[-4].regno), 24, 4); }
#line 3726 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 273:
#line 955 "./config/rx-parse.y" /* yacc.c:1648  */
    { rx_check_dfpu();
	  B3(0xfc, 0x78, 0x08); F((yyvsp[-1].regno), 16, 4); DSP((yyvsp[-3].exp), 14, DSIZE);
	  POST((yyvsp[-5].regno) << 4); }
#line 3734 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 274:
#line 959 "./config/rx-parse.y" /* yacc.c:1648  */
    { rx_check_dfpu();
	  B4(0xfc, 0xc8, 0x08, 0x00); F((yyvsp[-3].regno), 16, 4); F((yyvsp[0].regno), 24, 4); }
#line 3741 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 275:
#line 962 "./config/rx-parse.y" /* yacc.c:1648  */
    { rx_check_dfpu();
	  B3(0xfc, 0xc8, 0x08); F((yyvsp[-3].regno), 16, 4); DSP((yyvsp[-5].exp), 14, DSIZE);
	  POST((yyvsp[0].regno) << 4); }
#line 3749 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 276:
#line 966 "./config/rx-parse.y" /* yacc.c:1648  */
    { rx_check_dfpu();
	  B3(0xf9, 0x03, 0x03); F((yyvsp[0].regno), 16, 4); IMM((yyvsp[-2].exp), -1); }
#line 3756 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 277:
#line 969 "./config/rx-parse.y" /* yacc.c:1648  */
    { rx_check_dfpu();
	  B3(0xf9, 0x03, 0x02); F((yyvsp[0].regno), 16, 4); IMM((yyvsp[-2].exp), -1); }
#line 3763 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 278:
#line 972 "./config/rx-parse.y" /* yacc.c:1648  */
    { rx_check_dfpu();
	  B3(0xf9, 0x03, 0x00); F((yyvsp[0].regno), 16, 4); IMM((yyvsp[-2].exp), -1); }
#line 3770 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 279:
#line 975 "./config/rx-parse.y" /* yacc.c:1648  */
    { rx_check_dfpu();
	  B3(0x75, 0xb8, 0x00); F((yyvsp[-2].regno), 16, 4); F((yyvsp[0].regno) - (yyvsp[-2].regno), 20, 4); }
#line 3777 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 280:
#line 978 "./config/rx-parse.y" /* yacc.c:1648  */
    { rx_check_dfpu();
	  B3(0x75, 0xa8, 0x00); F((yyvsp[-2].regno), 16, 4); F((yyvsp[0].regno) - (yyvsp[-2].regno), 20, 4); }
#line 3784 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 281:
#line 981 "./config/rx-parse.y" /* yacc.c:1648  */
    { rx_check_dfpu();
	  B3(0x75, 0xb0, 0x00); F((yyvsp[-2].regno), 16, 4); F((yyvsp[0].regno) - (yyvsp[-2].regno), 20, 4); }
#line 3791 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 282:
#line 984 "./config/rx-parse.y" /* yacc.c:1648  */
    { rx_check_dfpu();
	  B3(0x75, 0xa0, 0x00); F((yyvsp[-2].regno), 16, 4); F((yyvsp[0].regno) - (yyvsp[-2].regno), 20, 4); }
#line 3798 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 283:
#line 987 "./config/rx-parse.y" /* yacc.c:1648  */
    { rx_check_dfpu();
	  B4(0xfd, 0x75, 0x80, 0x04); F((yyvsp[-2].regno), 24, 4); F((yyvsp[0].regno), 20, 4); }
#line 3805 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 284:
#line 990 "./config/rx-parse.y" /* yacc.c:1648  */
    { rx_check_dfpu(); B3(0x75, 0x90, 0x1b); }
#line 3811 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 285:
#line 992 "./config/rx-parse.y" /* yacc.c:1648  */
    { rx_check_dfpu();
	  B4(0xfd, 0x77, 0x80, 0x04); F((yyvsp[-2].regno), 24, 4); F((yyvsp[0].regno), 20, 4); }
#line 3818 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 286:
#line 995 "./config/rx-parse.y" /* yacc.c:1648  */
    { rx_check_dfpu();
	  B4(0xfd, 0x77, 0x80, 0x0a); F((yyvsp[-2].regno), 24, 4); F((yyvsp[0].regno), 20, 4); }
#line 3825 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 287:
#line 998 "./config/rx-parse.y" /* yacc.c:1648  */
    { rx_check_dfpu();
	  B4(0xfd, 0x77, 0x80, 0x09); F((yyvsp[-2].regno), 24, 4); F((yyvsp[0].regno), 20, 4); }
#line 3832 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 288:
#line 1001 "./config/rx-parse.y" /* yacc.c:1648  */
    { rx_check_dfpu();
	  B4(0xfd, 0x77, 0x80, 0x0d); F((yyvsp[-2].regno), 24, 4); F((yyvsp[0].regno), 20, 4); }
#line 3839 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 289:
#line 1012 "./config/rx-parse.y" /* yacc.c:1648  */
    { B2 (0x43 + (sub_op<<2), 0); F ((yyvsp[-2].regno), 8, 4); F ((yyvsp[0].regno), 12, 4); }
#line 3845 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 290:
#line 1014 "./config/rx-parse.y" /* yacc.c:1648  */
    { B2 (0x40 + (sub_op<<2), 0); F ((yyvsp[-4].regno), 8, 4); F ((yyvsp[0].regno), 12, 4); DSP ((yyvsp[-6].exp), 6, BSIZE); }
#line 3851 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 291:
#line 1016 "./config/rx-parse.y" /* yacc.c:1648  */
    { B3 (MEMEX, sub_op<<2, 0); F ((yyvsp[-2].regno), 8, 2); F ((yyvsp[-4].regno), 16, 4); F ((yyvsp[0].regno), 20, 4); DSP ((yyvsp[-6].exp), 14, sizemap[(yyvsp[-2].regno)]); }
#line 3857 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 292:
#line 1018 "./config/rx-parse.y" /* yacc.c:1648  */
    { id24 (4, sub_op<<4, 0), F ((yyvsp[0].regno), 12, 4), F ((yyvsp[-4].regno), 16, 4), F ((yyvsp[-2].regno), 20, 4); }
#line 3863 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 293:
#line 1025 "./config/rx-parse.y" /* yacc.c:1648  */
    { id24 (1, 0x03 + (sub_op<<2), 0x00); F ((yyvsp[-2].regno), 16, 4); F ((yyvsp[0].regno), 20, 4); }
#line 3869 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 294:
#line 1027 "./config/rx-parse.y" /* yacc.c:1648  */
    { B4 (MEMEX, 0xa0, 0x00 + sub_op, 0x00);
	  F ((yyvsp[-4].regno), 24, 4); F ((yyvsp[0].regno), 28, 4); DSP ((yyvsp[-6].exp), 14, LSIZE); }
#line 3876 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 295:
#line 1035 "./config/rx-parse.y" /* yacc.c:1648  */
    { id24 (1, 0x03 + (sub_op<<2), 0x00); F ((yyvsp[-2].regno), 16, 4); F ((yyvsp[0].regno), 20, 4); }
#line 3882 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 296:
#line 1037 "./config/rx-parse.y" /* yacc.c:1648  */
    { id24 (1, 0x00 + (sub_op<<2), 0x00); F ((yyvsp[-4].regno), 16, 4); F ((yyvsp[0].regno), 20, 4); DSP ((yyvsp[-6].exp), 14, BSIZE); }
#line 3888 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 297:
#line 1039 "./config/rx-parse.y" /* yacc.c:1648  */
    { B4 (MEMEX, 0x20 + ((yyvsp[-2].regno) << 6), 0x00 + sub_op, 0x00);
	  F ((yyvsp[-4].regno), 24, 4); F ((yyvsp[0].regno), 28, 4); DSP ((yyvsp[-6].exp), 14, sizemap[(yyvsp[-2].regno)]); }
#line 3895 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 298:
#line 1045 "./config/rx-parse.y" /* yacc.c:1648  */
    { id24 (2, 0x70, sub_op<<4); F ((yyvsp[0].regno), 20, 4); IMM ((yyvsp[-2].exp), 12); }
#line 3901 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 303:
#line 1060 "./config/rx-parse.y" /* yacc.c:1648  */
    { id24 (1, 0x03 + (sub_op<<2), 0x00); F ((yyvsp[-2].regno), 16, 4); F ((yyvsp[0].regno), 20, 4); }
#line 3907 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 304:
#line 1062 "./config/rx-parse.y" /* yacc.c:1648  */
    { B2 (0x7e, sub_op2 << 4); F ((yyvsp[0].regno), 12, 4); }
#line 3913 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 305:
#line 1067 "./config/rx-parse.y" /* yacc.c:1648  */
    { id24 (1, 0x4b + (sub_op2<<2), 0x00); F ((yyvsp[-2].regno), 16, 4); F ((yyvsp[0].regno), 20, 4); }
#line 3919 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 306:
#line 1071 "./config/rx-parse.y" /* yacc.c:1648  */
    { rx_check_v2 (); }
#line 3925 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 309:
#line 1079 "./config/rx-parse.y" /* yacc.c:1648  */
    { id24 (1, 0x03 + (sub_op<<2), 0); F ((yyvsp[-2].regno), 16, 4); F ((yyvsp[0].regno), 20, 4); }
#line 3931 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 310:
#line 1081 "./config/rx-parse.y" /* yacc.c:1648  */
    { id24 (1, 0x00 + (sub_op<<2), 0); F ((yyvsp[-4].regno), 16, 4); F ((yyvsp[0].regno), 20, 4); DSP ((yyvsp[-6].exp), 14, BSIZE); }
#line 3937 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 311:
#line 1083 "./config/rx-parse.y" /* yacc.c:1648  */
    { B4 (MEMEX, 0x20, 0x00 + sub_op, 0); F ((yyvsp[-2].regno), 8, 2); F ((yyvsp[-4].regno), 24, 4); F ((yyvsp[0].regno), 28, 4);
	    DSP ((yyvsp[-6].exp), 14, sizemap[(yyvsp[-2].regno)]); }
#line 3944 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 312:
#line 1090 "./config/rx-parse.y" /* yacc.c:1648  */
    { id24 (2, 0x60 + sub_op, 0); F ((yyvsp[-2].regno), 16, 4); F ((yyvsp[0].regno), 20, 4); }
#line 3950 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 313:
#line 1094 "./config/rx-parse.y" /* yacc.c:1648  */
    { B2 (0x68 + (sub_op<<1), 0); FE ((yyvsp[-2].exp), 7, 5); F ((yyvsp[0].regno), 12, 4); }
#line 3956 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 314:
#line 1096 "./config/rx-parse.y" /* yacc.c:1648  */
    { id24 (2, 0x80 + (sub_op << 5), 0); FE ((yyvsp[-4].exp), 11, 5); F ((yyvsp[-2].regno), 16, 4); F ((yyvsp[0].regno), 20, 4); }
#line 3962 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 316:
#line 1102 "./config/rx-parse.y" /* yacc.c:1648  */
    { rx_check_float_support (); id24 (2, 0x72, sub_op << 4); F ((yyvsp[0].regno), 20, 4); O4 ((yyvsp[-2].exp)); }
#line 3968 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 317:
#line 1104 "./config/rx-parse.y" /* yacc.c:1648  */
    { rx_check_float_support (); id24 (1, 0x83 + (sub_op << 2), 0); F ((yyvsp[-2].regno), 16, 4); F ((yyvsp[0].regno), 20, 4); }
#line 3974 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 318:
#line 1106 "./config/rx-parse.y" /* yacc.c:1648  */
    { rx_check_float_support (); id24 (1, 0x80 + (sub_op << 2), 0); F ((yyvsp[-4].regno), 16, 4); F ((yyvsp[0].regno), 20, 4); DSP ((yyvsp[-6].exp), 14, LSIZE); }
#line 3980 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 319:
#line 1108 "./config/rx-parse.y" /* yacc.c:1648  */
    { rx_check_v2 (); id24 (4, 0x80 + (sub_op << 4), 0 ); F ((yyvsp[-4].regno), 16, 4); F ((yyvsp[-2].regno), 20, 4); F ((yyvsp[0].regno), 12, 4); }
#line 3986 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 320:
#line 1112 "./config/rx-parse.y" /* yacc.c:1648  */
    { rx_check_float_support (); }
#line 3992 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 321:
#line 1114 "./config/rx-parse.y" /* yacc.c:1648  */
    { id24 (2, 0x72, sub_op << 4); F ((yyvsp[0].regno), 20, 4); O4 ((yyvsp[-2].exp)); }
#line 3998 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 323:
#line 1119 "./config/rx-parse.y" /* yacc.c:1648  */
    { rx_check_float_support (); }
#line 4004 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 324:
#line 1121 "./config/rx-parse.y" /* yacc.c:1648  */
    { id24 (1, 0x83 + (sub_op << 2), 0); F ((yyvsp[-2].regno), 16, 4); F ((yyvsp[0].regno), 20, 4); }
#line 4010 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 325:
#line 1122 "./config/rx-parse.y" /* yacc.c:1648  */
    { rx_check_float_support (); }
#line 4016 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 326:
#line 1124 "./config/rx-parse.y" /* yacc.c:1648  */
    { id24 (1, 0x80 + (sub_op << 2), 0); F ((yyvsp[-4].regno), 16, 4); F ((yyvsp[0].regno), 20, 4); DSP ((yyvsp[-6].exp), 14, LSIZE); }
#line 4022 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 327:
#line 1128 "./config/rx-parse.y" /* yacc.c:1648  */
    { rx_check_v2 (); }
#line 4028 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 328:
#line 1130 "./config/rx-parse.y" /* yacc.c:1648  */
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
#line 4051 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 330:
#line 1153 "./config/rx-parse.y" /* yacc.c:1648  */
    { rx_check_v3(); B3(0xff,0x60,0x00), F ((yyvsp[0].regno), 12, 4), F ((yyvsp[-4].regno), 16, 4), F ((yyvsp[-2].regno), 20, 4); }
#line 4057 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 331:
#line 1157 "./config/rx-parse.y" /* yacc.c:1648  */
    { rx_check_v3(); }
#line 4063 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 332:
#line 1159 "./config/rx-parse.y" /* yacc.c:1648  */
    { rx_range((yyvsp[-10].exp), 0, 31); rx_range((yyvsp[-7].exp), 0, 31); rx_range((yyvsp[-4].exp), 1, 31);
	    B3(0xfc, 0x5a + (sub_op << 2), 0); F((yyvsp[-2].regno), 16, 4); F((yyvsp[0].regno), 20, 4);
	  rx_bfield((yyvsp[-10].exp), (yyvsp[-7].exp), (yyvsp[-4].exp));}
#line 4071 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 333:
#line 1166 "./config/rx-parse.y" /* yacc.c:1648  */
    { B3(0xfd,0x76,0xe0 + (sub_op << 4)); UO1((yyvsp[0].exp)); }
#line 4077 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 334:
#line 1168 "./config/rx-parse.y" /* yacc.c:1648  */
    { B4(0xfd,0x76,0xc0 + (sub_op << 4), 0x00); F((yyvsp[0].regno), 20, 4); }
#line 4083 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 335:
#line 1173 "./config/rx-parse.y" /* yacc.c:1648  */
    { B4(0x76, 0x90, sub_op, sub_op2); F((yyvsp[-2].regno), 16, 4); F((yyvsp[0].regno), 24, 4);}
#line 4089 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 336:
#line 1177 "./config/rx-parse.y" /* yacc.c:1648  */
    { B4(0x76, 0x90, sub_op, 0x00); F((yyvsp[-4].regno), 28, 4); F((yyvsp[-2].regno), 16,4); F((yyvsp[0].regno), 24, 4);}
#line 4095 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 337:
#line 1181 "./config/rx-parse.y" /* yacc.c:1648  */
    { (yyval.exp) = zero_expr (); }
#line 4101 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 338:
#line 1182 "./config/rx-parse.y" /* yacc.c:1648  */
    { (yyval.exp) = (yyvsp[0].exp); }
#line 4107 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 339:
#line 1185 "./config/rx-parse.y" /* yacc.c:1648  */
    { need_flag = 1; }
#line 4113 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 340:
#line 1185 "./config/rx-parse.y" /* yacc.c:1648  */
    { need_flag = 0; (yyval.regno) = (yyvsp[0].regno); }
#line 4119 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 341:
#line 1190 "./config/rx-parse.y" /* yacc.c:1648  */
    { (yyval.regno) = 0; }
#line 4125 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 342:
#line 1191 "./config/rx-parse.y" /* yacc.c:1648  */
    { (yyval.regno) = 1; }
#line 4131 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 343:
#line 1192 "./config/rx-parse.y" /* yacc.c:1648  */
    { (yyval.regno) = 2; }
#line 4137 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 344:
#line 1193 "./config/rx-parse.y" /* yacc.c:1648  */
    { (yyval.regno) = 2; }
#line 4143 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 345:
#line 1194 "./config/rx-parse.y" /* yacc.c:1648  */
    { (yyval.regno) = 3; }
#line 4149 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 346:
#line 1197 "./config/rx-parse.y" /* yacc.c:1648  */
    { (yyval.regno) = LSIZE; }
#line 4155 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 347:
#line 1198 "./config/rx-parse.y" /* yacc.c:1648  */
    { (yyval.regno) = BSIZE; }
#line 4161 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 348:
#line 1199 "./config/rx-parse.y" /* yacc.c:1648  */
    { (yyval.regno) = WSIZE; }
#line 4167 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 349:
#line 1200 "./config/rx-parse.y" /* yacc.c:1648  */
    { (yyval.regno) = LSIZE; }
#line 4173 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 350:
#line 1203 "./config/rx-parse.y" /* yacc.c:1648  */
    { (yyval.regno) = 1; }
#line 4179 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 351:
#line 1204 "./config/rx-parse.y" /* yacc.c:1648  */
    { (yyval.regno) = 0; }
#line 4185 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 352:
#line 1205 "./config/rx-parse.y" /* yacc.c:1648  */
    { (yyval.regno) = 1; }
#line 4191 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 353:
#line 1208 "./config/rx-parse.y" /* yacc.c:1648  */
    {}
#line 4197 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 354:
#line 1209 "./config/rx-parse.y" /* yacc.c:1648  */
    {}
#line 4203 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 355:
#line 1212 "./config/rx-parse.y" /* yacc.c:1648  */
    {}
#line 4209 "rx-parse.c" /* yacc.c:1648  */
    break;

  case 356:
#line 1213 "./config/rx-parse.y" /* yacc.c:1648  */
    {}
#line 4215 "rx-parse.c" /* yacc.c:1648  */
    break;


#line 4219 "rx-parse.c" /* yacc.c:1648  */
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
#line 1216 "./config/rx-parse.y" /* yacc.c:1907  */

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

  { "dr0", DREG, 0 },
  { "dr1", DREG, 1 },
  { "dr2", DREG, 2 },
  { "dr3", DREG, 3 },
  { "dr4", DREG, 4 },
  { "dr5", DREG, 5 },
  { "dr6", DREG, 6 },
  { "dr7", DREG, 7 },
  { "dr8", DREG, 8 },
  { "dr9", DREG, 9 },
  { "dr10", DREG, 10 },
  { "dr11", DREG, 11 },
  { "dr12", DREG, 12 },
  { "dr13", DREG, 13 },
  { "dr14", DREG, 14 },
  { "dr15", DREG, 15 },
  
  { "drh0", DREGH, 0 },
  { "drh1", DREGH, 1 },
  { "drh2", DREGH, 2 },
  { "drh3", DREGH, 3 },
  { "drh4", DREGH, 4 },
  { "drh5", DREGH, 5 },
  { "drh6", DREGH, 6 },
  { "drh7", DREGH, 7 },
  { "drh8", DREGH, 8 },
  { "drh9", DREGH, 9 },
  { "drh10", DREGH, 10 },
  { "drh11", DREGH, 11 },
  { "drh12", DREGH, 12 },
  { "drh13", DREGH, 13 },
  { "drh14", DREGH, 14 },
  { "drh15", DREGH, 15 },

  { "drl0", DREGL, 0 },
  { "drl1", DREGL, 1 },
  { "drl2", DREGL, 2 },
  { "drl3", DREGL, 3 },
  { "drl4", DREGL, 4 },
  { "drl5", DREGL, 5 },
  { "drl6", DREGL, 6 },
  { "drl7", DREGL, 7 },
  { "drl8", DREGL, 8 },
  { "drl9", DREGL, 9 },
  { "drl10", DREGL, 10 },
  { "drl11", DREGL, 11 },
  { "drl12", DREGL, 12 },
  { "drl13", DREGL, 13 },
  { "drl14", DREGL, 14 },
  { "drl15", DREGL, 15 },

  { "DPSW", DCREG, 0 },
  { "DCMR", DCREG, 1 },
  { "DCENT", DCREG, 2 },
  { "DEPC", DCREG, 3 },
  { "DCR0", DCREG, 0 },
  { "DCR1", DCREG, 1 },
  { "DCR2", DCREG, 2 },
  { "DCR3", DCREG, 3 },
  
  { ".s", DOT_S, 0 },
  { ".b", DOT_B, 0 },
  { ".w", DOT_W, 0 },
  { ".l", DOT_L, 0 },
  { ".a", DOT_A , 0},
  { ".ub", DOT_UB, 0 },
  { ".uw", DOT_UW , 0},
  { ".d", DOT_D , 0},

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
  OPC(BFMOV),
  OPC(BFMOVZ),
  OPC(BMCND),
  OPC(BNOT),
  OPC(BRA),
  OPC(BRK),
  OPC(BSET),
  OPC(BSR),
  OPC(BTST),
  OPC(CLRPSW),
  OPC(CMP),
  OPC(DABS),
  OPC(DADD),
  OPC(DBT),
  OPC(DDIV),
  OPC(DIV),
  OPC(DIVU),
  OPC(DMOV),
  OPC(DMUL),
  OPC(DNEG),
  OPC(DPOPM),
  OPC(DPUSHM),
  OPC(DROUND),
  OPC(DSQRT),
  OPC(DSUB),
  OPC(DTOF),
  OPC(DTOI),
  OPC(DTOU),
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
  OPC(FTOD),
  OPC(FTOU),
  OPC(FSUB),
  OPC(FTOI),
  OPC(INT),
  OPC(ITOD),
  OPC(ITOF),
  OPC(JMP),
  OPC(JSR),
  OPC(MVFACGU),
  OPC(MVFACHI),
  OPC(MVFACMI),
  OPC(MVFACLO),
  OPC(MVFC),
  OPC(MVFDC),
  OPC(MVFDR),
  OPC(MVTDC),
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
  OPC(RSTR),
  OPC(RTE),
  OPC(RTFI),
  OPC(RTS),
  OPC(RTSD),
  OPC(SAT),
  OPC(SATR),
  OPC(SAVE),
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
  OPC(UTOD),
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

struct condition_symbol
{
  const char * string;
  int    val;
};

static struct condition_symbol condition_table[] =
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
  { "no", 13 },
  /* never = 15 */
};

static struct condition_symbol double_condition_table[] =
{
  { "un", 1 },
  { "eq", 2 },
  { "lt", 4 },
  { "le", 6 },
};

#define NUM_CONDITIONS (sizeof (condition_table) / sizeof (condition_table[0]))
#define NUM_DOUBLE_CONDITIONS (sizeof (double_condition_table) / sizeof (double_condition_table[0]))

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
check_condition (const char * base, struct condition_symbol *t, unsigned int num)
{
  char * cp;
  unsigned int i;

  if ((unsigned) (rx_lex_end - rx_lex_start) < strlen (base) + 1)
    return 0;
  if (memcmp (rx_lex_start, base, strlen (base)))
    return 0;
  cp = rx_lex_start + strlen (base);
  for (i = 0; i < num; i ++)
    {
      if (strcasecmp (cp, t[i].string) == 0)
	{
	  rx_lval.regno = t[i].val;
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
	{
	  for (ci = 0; ci < NUM_CONDITION_OPCODES; ci ++)
	    if (check_condition (condition_opcode_table[ci].string,
				 condition_table, NUM_CONDITIONS))
	      {
		*e = save;
		rx_lex_start = e;
		rx_last_token = condition_opcode_table[ci].token;
		return condition_opcode_table[ci].token;
	      }
	  if  (check_condition ("dcmp", double_condition_table,
				NUM_DOUBLE_CONDITIONS))
	    {
	      *e = save;
	      rx_lex_start = e;
	      rx_last_token = DCMP;
	      return DCMP;
	    }
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
      || rx_last_token == REG || rx_last_token == DREG || rx_last_token == DCREG
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

  if (pos >= 0 && rx_intop (exp, 8, bits))
    {
      rx_op (exp, 1, type);
      return 1;
    }
  else if (pos >= 0 && rx_intop (exp, 16, bits))
    {
      rx_op (exp, 2, type);
      return 2;
    }
  else if (pos >= 0 && rx_uintop (exp, 16) && bits == 16)
    {
      rx_op (exp, 2, type);
      return 2;
    }
  else if (pos >= 0 && rx_intop (exp, 24, bits))
    {
      rx_op (exp, 3, type);
      return 3;
    }
  else if (pos < 0 || rx_intop (exp, 32, bits))
    {
      rx_op (exp, 4, type);
      return 0;
    }
  else if (type == RXREL_SIGNED && pos >= 0)
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
    case DSIZE:
      if (val & 7)
	rx_error (_("double displacement not double-aligned"));
      vshift = 3;
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

static void
rx_check_v3 (void)
{
  if (rx_cpu < RXV3)
    rx_error (_("target CPU type does not support v3 instructions"));
}

static void
rx_check_dfpu (void)
{
  if (rx_cpu != RXV3FPU)
    rx_error (_("target CPU type does not support double float instructions"));
}
